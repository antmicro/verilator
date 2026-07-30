import json
import os
import subprocess
import shutil
import time
from pathlib import Path
import sys
from datetime import datetime

import matplotlib.pyplot as plt
from matplotlib.ticker import MaxNLocator

import numpy as np
from scipy.stats import entropy, chi2 as scipy_chi2

INPUT_DIR  = Path(__file__).resolve().parent.parent / 'input'
OUTPUT_DIR = Path(__file__).resolve().parent.parent / 'output'
REPO_ROOT  = Path(__file__).resolve().parent.parent.parent

MAX_DEFAULT_ITERATIONS = 5_000_000
_SAMPLES_PER_SOLUTION  = int(os.environ.get('SAMPLES_PER_SOLUTION', 25))


def compute_default_iterations(num_solutions):
    """Return default iteration count: SAMPLES_PER_SOLUTION * num_solutions, capped at MAX_DEFAULT_ITERATIONS."""
    return min(_SAMPLES_PER_SOLUTION * num_solutions, MAX_DEFAULT_ITERATIONS)

def verilator_binary():
    """Path to the verilator binary to test.
    Uses VERILATOR_CNT_RND if set, otherwise this repo's own built bin/verilator.
    """
    return os.environ.get('VERILATOR_CNT_RND', str(REPO_ROOT / 'bin' / 'verilator'))

def compile_sv(obj_dir, num_iterations, input_path):
    """ Uses Verilator $VERILATOR_CNSTR_RND,
    and compiles the SV benchmark.
    Returns elapsed wall-clock time in seconds.
    """
    shutil.rmtree(obj_dir, ignore_errors=True)
    iterations = '+define+NUM_ITERATIONS=%d' % num_iterations
    cmd = [verilator_binary(), "--binary", "-j", "0"]
    cmd += ['--Mdir', str(obj_dir), str(iterations), str(input_path)]
    t0 = time.monotonic()
    result = subprocess.run(cmd, capture_output=True, text=True)
    elapsed = time.monotonic() - t0
    if result.returncode != 0:
        print('Verilator compilation failed:')
        print(result.stderr)
        raise SystemExit(1)
    print('Compiled: %s (%.1fs)' % (input_path.name, elapsed))
    return elapsed

def simulate(sim_path):
    """Run the simulation binary and return (lines, elapsed_seconds)."""
    t0 = time.monotonic()
    result = subprocess.run([str(sim_path)], capture_output=True, text=True)
    elapsed = time.monotonic() - t0
    if result.returncode != 0:
        print('Simulation failed:')
        print(result.stdout)
        print(result.stderr)
        raise SystemExit(1)
    lines = []
    for line in result.stdout.splitlines():
        stripped = line.strip()
        if stripped:
            lines.append(stripped)
    print('Simulated: %.1fs' % elapsed)
    return lines, elapsed

def count_uniform_samples(iterations, solutions):
    """Return a dict mapping each solution to its expected count under a uniform distribution.
    Expected count = iterations / len(solutions), as a float.
    """
    if not solutions:
        raise ValueError('solutions must not be empty')
    expected_count = iterations / len(solutions)
    return {s: expected_count for s in solutions}

def count_unsat_samples(lines):
    """Count lines that are exactly '-UNSAT', emitted when randomize() fails."""
    return sum(1 for line in lines if line == '-UNSAT')


def count_observed_samples(lines):
    """Return a dict mapping each observed value to its count.
    Single-variable lines are parsed as int keys.
    Multi-variable lines (space-separated integers) are kept as string keys,
    matching the string encoding used in ALL_SOLUTIONS for those benchmarks.
    Fails loudly on any line that is neither an integer nor a known Verilator meta-line.
    Known meta-lines: lines starting with '-' (e.g. $finish messages).
    """
    counts = {}
    for line in lines:
        if line.startswith('-'):
            continue  # Verilator info/finish message
        parts = line.split()
        if not all(p.lstrip('-').isdigit() for p in parts):
            raise ValueError(f'Unexpected output line: {line!r}')
        key = int(parts[0]) if len(parts) == 1 else line
        counts[key] = counts.get(key, 0) + 1
    return counts

def compute_coverage_curve(lines, solutions):
    """Compute cumulative coverage after each valid sample.
    Returns (x, y) where x[i] = sample index (1-based) and y[i] = coverage fraction.
    Only valid lines (not Verilator meta-lines) are counted.
    Downsampled to at most 2000 points for large N to keep SVG size manageable.
    """
    solution_set = set(solutions)
    seen = set()
    xs, ys = [], []
    idx = 0
    for line in lines:
        if line.startswith('-'):
            continue
        parts = line.split()
        key = int(parts[0]) if len(parts) == 1 else line
        if key in solution_set:
            seen.add(key)
        idx += 1
        xs.append(idx)
        ys.append(len(seen) / len(solution_set))
    if not xs:
        return xs, ys
    # Downsample: keep at most 2000 evenly-spaced points plus the last point
    if len(xs) > 2000:
        step = len(xs) // 2000
        indices = list(range(0, len(xs), step))
        if indices[-1] != len(xs) - 1:
            indices.append(len(xs) - 1)
        xs = [xs[i] for i in indices]
        ys = [ys[i] for i in indices]
    return xs, ys


def draw_coverage_curve(x, y, module_name, num_solutions, num_iterations, out_path):
    """Plot cumulative coverage (fraction of valid solutions seen) vs. sample index."""
    plt.figure()
    plt.plot(x, y, color='steelblue', linewidth=1.2)
    plt.axhline(y=1.0, color='gray', linestyle='--', linewidth=0.8, label='100% coverage')
    plt.xlabel('Sample index')
    plt.ylabel('Coverage (fraction of solutions seen)')
    plt.title(f'{module_name} — coverage curve\n({num_iterations} samples, {num_solutions} solutions)')
    plt.ylim(0, 1.05)
    plt.xlim(0, num_iterations)
    plt.legend()
    plt.tight_layout()
    plt.savefig(str(out_path))
    plt.close()
    print('Coverage curve saved under: ' + str(out_path))


def draw_frequency_plot(observed_samples, uniform_samples, module_name, out_path):
    """Plot observed counts as points and expected uniform counts as a horizontal line.
    X-axis shows solution index (1-based) rather than solution values.
    """
    solutions = sorted(uniform_samples.keys())
    n = len(solutions)
    indices = list(range(1, n + 1))
    observed = [observed_samples.get(s, 0) for s in solutions]
    expected = [uniform_samples[s] for s in solutions]

    # Pick ~10 evenly spaced tick positions; always include 1 and n.
    step = max(1, n // 10)
    tick_positions = list(range(1, n + 1, step))
    if tick_positions[-1] != n:
        tick_positions.append(n)

    expected_count = int(expected[0])  # always integer after rounding

    plt.figure()
    plt.axhline(y=expected_count, color='blue', linestyle='--', label='Uniform expected')
    plt.plot(indices, observed, 'o', color='red', label='Observed', markersize=3)
    plt.xlabel('Solution index')
    plt.ylabel('Count')
    plt.title(f'{module_name} - frequency plot (n={sum(observed)}, {n} solutions)')
    plt.legend(bbox_to_anchor=(0.5, -0.12), loc='upper center', ncol=2)
    plt.xticks(tick_positions)

    ax = plt.gca()
    ax.yaxis.set_major_locator(MaxNLocator(integer=True))
    plt.draw()  # force tick computation before reading current ticks
    current_yticks = list(ax.get_yticks())
    if expected_count not in current_yticks:
        current_yticks.append(expected_count)
        current_yticks.sort()
        ax.set_yticks(current_yticks)

    plt.tight_layout()
    plt.savefig(str(out_path))
    print('Plot saved under: ' + str(out_path))

def calculate_coverage(solutions, observed_samples):
    """Compute coverage: fraction of valid solutions observed at least once.
    Returns (distinct_hit, total, coverage_ratio).
    """
    total = len(solutions)
    distinct_hit = sum(1 for s in solutions if s in observed_samples)
    return distinct_hit, total, distinct_hit / total


def calculate_variance(solutions, observed_samples):
    """Compute variance of per-solution relative frequencies.
    f_i = c_i / N for each solution i; f_i = 0 for unobserved solutions.
    Source: Chakraborty et al. — "the smaller the scaled variance,
    the more uniform is the generated distribution."
    """
    n_samples = sum(observed_samples.values())
    if n_samples == 0:
        raise ValueError('No samples collected')
    freqs = np.array([observed_samples.get(s, 0) / n_samples for s in solutions])
    return float(np.var(freqs)) * 10000


def calculate_jsd(solutions, observed_samples, uniform_samples):
    """Compute Jensen-Shannon divergence JSD(P‖Q) in nats,
    where P = uniform and Q = observed.
    JSD = (KL(P‖M) + KL(Q‖M)) / 2, M = (P + Q) / 2.
    Always finite and bounded in [0, ln(2)] nats (~0.693).
    """
    n_samples = sum(observed_samples.values())
    n_uniform = sum(uniform_samples.values())
    if n_samples == 0:
        raise ValueError('No samples collected')
    P = np.array([uniform_samples[s] / n_uniform for s in solutions])
    Q = np.array([observed_samples.get(s, 0) / n_samples for s in solutions])
    M = (P + Q) / 2
    return float((entropy(P, M) + entropy(Q, M)) / 2) * 100


def calculate_chi_square(solutions, observed_samples, n_samples):
    """Compute Pearson chi-square p-value against the uniform distribution.
    Expected count per solution = N / M.
    Returns the p-value: closer to 1 means closer to uniform.
    Returns float('nan') when expected count < 5 (test is statistically unreliable).
    Source: Pesant et al. — "the closer to 1 the p-value, the closer to uniform."

    Uses manual chi-square stat computation to avoid scipy >= 1.7 ValueError
    caused by floating-point sum mismatch when passing f_exp to chisquare().
    """
    m = len(solutions)
    expected_count = n_samples / m
    if expected_count < 5:
        print(f'WARNING: chi-square expected count {expected_count:.2f} < 5 — p-value set to NaN')
        return float('nan')
    observed_counts = np.array([observed_samples.get(s, 0) for s in solutions], dtype=float)
    chi2_stat = float(np.sum((observed_counts - expected_count) ** 2 / expected_count))
    p_value = scipy_chi2.sf(chi2_stat, df=m - 1)
    return float(p_value)


def calculate_kl(uniform_samples, observed_samples):
    """Compute KL divergence KL(P‖Q) in nats, where P = uniform and Q = observed.
    Both dicts map solution -> count. Counts are normalized internally.
    """
    if not observed_samples:
        raise ValueError('observed_samples is empty — no valid samples collected')
    observed_samples_total_count = sum(observed_samples.values())
    uniform_samples_total_count = sum(uniform_samples.values())
    if observed_samples_total_count == 0:
        raise ValueError('observed_samples total count is zero')
    if uniform_samples_total_count == 0:
        raise ValueError('uniform_samples total count is zero')
    solutions = sorted(uniform_samples.keys())

    P = np.array([uniform_samples[s] / uniform_samples_total_count for s in solutions]) # uniform distribution
    Q = np.array([observed_samples.get(s,0) / observed_samples_total_count for s in solutions]) # observed distribution
    kl_nats = entropy(P, Q)
    return kl_nats * 100

def parse_iterations():
    """Read optional iteration count from the command line.
    Returns None when not provided — run_experiment will then use compute_default_iterations().
    """
    if len(sys.argv) == 1:
        return None
    if len(sys.argv) != 2:
        print('Usage: python3 run_<name>.py [num_iterations]')
        raise SystemExit(1)
    try:
        value = int(sys.argv[1])
    except ValueError:
        print('num_iterations must be an integer')
        raise SystemExit(1)
    if value < 1:
        print('num_iterations must be >= 1')
        raise SystemExit(1)
    return value

_METHOD_NAME = os.environ.get('VERILATOR_METHOD', 'verilator')
RST_FILE     = f'{_METHOD_NAME}-distribution.rst'
METRICS_FILE = f'{_METHOD_NAME}-metrics.jsonl'

REPORT_DESCRIPTION = (
    'This report evaluates how close the distribution produced by Verilator '
    'constrained randomization is to the ideal uniform distribution, '
    'on small fully enumerable benchmarks. '
    'Each benchmark is run repeatedly; the empirical sample distribution is '
    'compared against the uniform distribution over all valid solutions.'
)


def _get_verilator_info():
    """Return a string describing the Verilator version and commit SHA.
    Uses VERILATOR_COMMIT_SHA env var for the exact commit (set by CI).
    Falls back to parsing `verilator --version` output.
    """
    commit_sha = os.environ.get('VERILATOR_COMMIT_SHA', '')
    version_line = ''
    try:
        result = subprocess.run(
            [verilator_binary(), '--version'], capture_output=True, text=True, timeout=5
        )
        if result.returncode == 0:
            version_line = result.stdout.strip().splitlines()[0]
    except Exception:
        pass
    if commit_sha:
        if version_line:
            return '%s (commit ``%s``)' % (version_line, commit_sha)
        return 'commit ``%s``' % commit_sha
    return version_line or 'unknown'


def write_report_header(iterations):
    """Write the top-level RST header. Creates the report file (overwrites if exists)."""
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    date = datetime.now().strftime('%Y-%m-%d %H:%M')
    commit = os.environ.get('CI_COMMIT_SHA', 'unknown')
    verilator_info = _get_verilator_info()
    title = f'Verilator Distribution Analysis — {_METHOD_NAME}'
    with open(OUTPUT_DIR / RST_FILE, 'w') as f:
        f.write('=' * len(title) + '\n')
        f.write(title + '\n')
        f.write('=' * len(title) + '\n\n')
        f.write(':Date: %s\n' % date)
        f.write(':Commit: ``%s``\n' % commit)
        f.write(':Verilator: %s\n' % verilator_info)
        if iterations is None:
            iter_str = 'per-benchmark default (%d x num_solutions, max %d)' % (_SAMPLES_PER_SOLUTION, MAX_DEFAULT_ITERATIONS)
        else:
            iter_str = str(iterations)
        f.write(':Iterations: %s\n\n' % iter_str)
        f.write(REPORT_DESCRIPTION + '\n\n')


def _extract_sv_description(input_path):
    """Return the first non-empty // comment line from the SV file."""
    try:
        for line in input_path.read_text().splitlines():
            stripped = line.strip()
            if stripped.startswith('//'):
                desc = stripped.lstrip('/').strip()
                if desc:
                    return desc
    except Exception:
        pass
    return ''


def generate_report(module_name, solutions, iterations, observed, kl_nats, jsd_nats, coverage, distinct_hit, freq_variance, chi2_p, compile_time, sim_time, input_path, method='verilator', unsat_pct=0.0, uniqueness_pct=100.0, freq_svg_name=None, cov_svg_name=None):
    """Append a per-experiment section to the RST report.
    If the report file does not exist yet, write the top-level header first.
    """
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    rst_path = OUTPUT_DIR / RST_FILE
    num_solutions = len(solutions)
    num_samples = sum(observed.values())
    uniform_pct = 100.0 / num_solutions

    if not rst_path.exists():
        write_report_header(iterations)

    sv_source = input_path.read_text()

    with open(rst_path, 'a') as f:
        subtitle = module_name
        f.write(subtitle + '\n')
        f.write('-' * len(subtitle) + '\n\n')

        f.write(':Samples: %d\n' % num_samples)
        f.write(':Possible solutions: %d\n' % num_solutions)
        f.write(':Uniform expected: %.2f%% each\n\n' % uniform_pct)

        f.write('.. dropdown:: View source: %s\n\n' % input_path.name)
        f.write('   .. code-block:: systemverilog\n\n')
        for line in sv_source.splitlines():
            f.write('      ' + line + '\n')
        f.write('\n')

        f.write('Results\n')
        f.write('~~~~~~~\n\n')
        f.write(':Coverage: %d/%d (%.2f%%)\n' % (distinct_hit, num_solutions, coverage * 100))
        f.write(':Uniqueness: %.2f%% of successful samples are distinct\n' % uniqueness_pct)
        f.write(':UNSAT rate: %.2f%% of randomize() calls failed\n' % unsat_pct)
        f.write(':Variance of relative frequencies: %.8f\n (multiplied by 10000)' % freq_variance)
        f.write(':KL divergence: **%.6f** (nats * 100)\n' % kl_nats)
        f.write(':Jensen-Shannon divergence: **%.6f** (nats * 100)\n' % jsd_nats)
        if np.isnan(chi2_p):
            chi2_str = 'N/A (expected count < 5)'
        elif chi2_p < 1e-4:
            chi2_str = '**%.2e** (closer to 1 = more uniform)' % chi2_p
        else:
            chi2_str = '**%.6f** (closer to 1 = more uniform)' % chi2_p
        f.write(':Pearson chi-square p-value: %s\n' % chi2_str)
        f.write(':Verilation time: %.1fs\n' % compile_time)
        f.write(':Simulation time: %.1fs\n\n' % sim_time)

        f.write('.. dropdown:: View frequency plot\n\n')
        f.write('   .. image:: %s\n\n' % (freq_svg_name or f'{method}_{module_name}_frequency.svg'))

        f.write('.. dropdown:: View coverage curve\n\n')
        f.write('   .. image:: %s\n\n' % (cov_svg_name or f'{method}_{module_name}_coverage.svg'))

    metrics_path = OUTPUT_DIR / METRICS_FILE
    entry = {
        'method':         method,
        'module_name':    module_name,
        'num_solutions':  num_solutions,
        'num_samples':    num_samples,
        'coverage_pct':   coverage * 100,
        'distinct_hit':   distinct_hit,
        'uniqueness_pct': uniqueness_pct,
        'unsat_pct':      unsat_pct,
        'kl':             kl_nats,
        'jsd':            jsd_nats,
        'variance':       freq_variance,
        'chi2_p':         None if np.isnan(chi2_p) else chi2_p,
        'compile_time':   round(compile_time, 1),
        'sim_time':       round(sim_time, 1),
        'description':    _extract_sv_description(input_path),
    }
    with open(metrics_path, 'a') as mf:
        mf.write(json.dumps(entry) + '\n')


def write_summary_table():
    """Read metrics.jsonl and append a summary RST table to the report.
    Called by run_all.py after all sub-experiments complete.
    """
    metrics_path = OUTPUT_DIR / METRICS_FILE
    if not metrics_path.exists():
        return
    rows = []
    with open(metrics_path) as mf:
        for line in mf:
            line = line.strip()
            if line:
                rows.append(json.loads(line))
    if not rows:
        return

    def fmt_chi2(v):
        if v is None:
            return 'N/A'
        if v < 1e-4:
            return '%.2e' % v
        return '%.6f' % v

    jsd_vals  = [r['jsd']      for r in rows]
    var_vals  = [r['variance']  for r in rows]
    chi2_vals = [r['chi2_p']   for r in rows if r['chi2_p'] is not None]

    avg_jsd  = float(np.mean(jsd_vals))
    avg_var  = float(np.mean(var_vals))
    avg_chi2 = float(np.mean(chi2_vals)) if chi2_vals else None

    rst_path = OUTPUT_DIR / RST_FILE
    with open(rst_path, 'a') as f:
        title = 'Summary'
        f.write(title + '\n')
        f.write('-' * len(title) + '\n\n')

        f.write('.. list-table::\n')
        f.write('   :header-rows: 1\n')
        f.write('   :widths: 24 8 8 10 10 10 10 8 8 34\n\n')
        f.write('   * - Benchmark\n')
        f.write('     - Solutions\n')
        f.write('     - Coverage\n')
        f.write('     - KL (x100)\n')
        f.write('     - JSD (x100)\n')
        f.write('     - Var (x10000)\n')
        f.write('     - chi2 p-value\n')
        f.write('     - Veril. (s)\n')
        f.write('     - Sim. (s)\n')
        f.write('     - Description\n')
        for r in rows:
            f.write('   * - ``%s``\n'  % r['module_name'])
            f.write('     - %d\n'       % r['num_solutions'])
            f.write('     - %.1f%%\n'   % r['coverage_pct'])
            f.write('     - %.6f\n'     % r['kl'])
            f.write('     - %.6f\n'     % r['jsd'])
            f.write('     - %.8f\n'     % r['variance'])
            f.write('     - %s\n'       % fmt_chi2(r['chi2_p']))
            f.write('     - %.1f\n'     % r.get('compile_time', 0))
            f.write('     - %.1f\n'     % r.get('sim_time', 0))
            f.write('     - %s\n'       % r['description'])
        f.write('\n')

        total_compile = sum(r.get('compile_time', 0) for r in rows)
        total_sim = sum(r.get('sim_time', 0) for r in rows)

        f.write('**Averages across all benchmarks:**\n\n')
        f.write(':Average JSD (x100): %.6f\n'        % avg_jsd)
        f.write(':Average Var (x10000): %.8f\n'      % avg_var)
        f.write(':Average chi2 p-value: %s\n'        % fmt_chi2(avg_chi2))
        f.write(':Total verilation time: %.1fs\n'    % total_compile)
        f.write(':Total simulation time: %.1fs\n\n'  % total_sim)


def round_iterations(iterations, num_solutions):
    """Round iterations to the nearest multiple of num_solutions.
    Ensures iterations / num_solutions is always an exact integer,
    so the expected uniform count is never a float.
    Always returns at least num_solutions (minimum one full round).
    """
    if iterations < num_solutions:
        return num_solutions
    remainder = iterations % num_solutions
    if remainder == 0:
        return iterations
    # Round to nearest multiple (up or down)
    lower = iterations - remainder
    upper = lower + num_solutions
    return lower if remainder < num_solutions / 2 else upper


def run_experiment(module_name, solutions, iterations=None, method=None):
    if method is None:
        method = os.environ.get('VERILATOR_METHOD', 'verilator')
    requested = iterations if iterations is not None else compute_default_iterations(len(solutions))
    ITERATIONS = round_iterations(requested, len(solutions))
    if ITERATIONS != requested:
        print(f'Iterations rounded to {ITERATIONS} (nearest multiple of {len(solutions)})')

    INPUT_PATH = INPUT_DIR / (module_name + ".sv")
    if not INPUT_PATH.exists():
        print(f'Input file not found: {INPUT_PATH}')
        raise SystemExit(1)
    OBJ_DIR = OUTPUT_DIR / ("build_" + module_name)
    SIM_PATH = OBJ_DIR / ("V" + module_name)

    freq_svg_name = f'{method}_{module_name}_frequency.svg'
    cov_svg_name  = f'{method}_{module_name}_coverage.svg'
    freq_svg_path = OUTPUT_DIR / freq_svg_name
    cov_svg_path  = OUTPUT_DIR / cov_svg_name

    compile_time = compile_sv(OBJ_DIR, ITERATIONS, INPUT_PATH)
    lines, sim_time = simulate(SIM_PATH)
    observed = count_observed_samples(lines)
    unsat_count = count_unsat_samples(lines)
    uniform = count_uniform_samples(ITERATIONS, solutions)
    draw_frequency_plot(observed, uniform, module_name, freq_svg_path)
    cov_x, cov_y = compute_coverage_curve(lines, solutions)
    draw_coverage_curve(cov_x, cov_y, module_name, len(solutions), ITERATIONS, cov_svg_path)
    kl_nats = calculate_kl(uniform, observed)
    jsd_nats = calculate_jsd(solutions, observed, uniform)
    distinct_hit, total, coverage = calculate_coverage(solutions, observed)
    freq_variance = calculate_variance(solutions, observed)
    n_samples = sum(observed.values())
    total_attempts = n_samples + unsat_count
    unsat_pct = 100.0 * unsat_count / total_attempts if total_attempts > 0 else 0.0
    uniqueness_pct = 100.0 * distinct_hit / n_samples if n_samples > 0 else 0.0
    chi2_p = calculate_chi_square(solutions, observed, n_samples)
    print('\nKL divergence for %s: %.6f (nats * 100)' % (INPUT_PATH.name, kl_nats))
    print('Jensen-Shannon divergence: %.6f (nats * 100)' % jsd_nats)
    print('Coverage: %d/%d (%.2f%%)' % (distinct_hit, total, coverage * 100))
    print('Variance of relative frequencies: %.8f (multiplied by 10000)' % freq_variance)
    if np.isnan(chi2_p):
        chi2_str = 'N/A (expected count < 5)'
    elif chi2_p < 1e-4:
        chi2_str = '%.2e' % chi2_p
    else:
        chi2_str = '%.6f' % chi2_p
    print('Pearson chi-square p-value: %s' % chi2_str)
    print('Verilation time: %.1fs' % compile_time)
    print('Simulation time: %.1fs' % sim_time)
    generate_report(module_name, solutions, ITERATIONS, observed, kl_nats, jsd_nats, coverage, distinct_hit, freq_variance, chi2_p, compile_time, sim_time, INPUT_PATH, method=method, unsat_pct=unsat_pct, uniqueness_pct=uniqueness_pct, freq_svg_name=freq_svg_name, cov_svg_name=cov_svg_name)
