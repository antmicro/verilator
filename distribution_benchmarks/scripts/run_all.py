import sys
import shutil
import subprocess
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import parse_iterations, write_report_header, write_summary_table

SCRIPTS_DIR = Path(__file__).resolve().parent
OUTPUT_DIR  = Path(__file__).resolve().parent.parent / 'output'

# Benchmarks over scalar rand variables, grouped by the property they vary.
SCALAR_RUNNERS = [
    # Dim1 — constraint type
    'run_distr_range_1000.py',           # range (anchor baseline)
    'run_distr_modulo_1000.py',          # modulo
    'run_distr_primes_1000.py',          # explicit set membership (OR-chain of equalities)
    'run_distr_countones.py',            # bitwise ($countones)
    'run_distr_quadratic_100.py',        # quadratic
    'run_distr_abs_diff_100.py',         # absolute difference of two vars
    'run_distr_digit_sum.py',            # decimal digit sum
    'run_distr_implication.py',          # implication (->)
    'run_distr_conditional_bias.py',     # if/else branches
    # Dim2 — solution space size
    'run_distr_range_10.py',             # tiny (10)
    'run_distr_range_100.py',            # small (100)
    'run_distr_modulo_100k.py',          # large (~100k), modular
    'run_distr_range_1M.py',             # large (1M), trivial range
    # Dim3 — solution space shape
    'run_distr_two_choice.py',           # two solutions only
    'run_distr_unequal_bins.py',         # unequal bins (100x ratio)
    'run_distr_mixed_9989.py',           # mixed dense/sparse regions
    'run_distr_sparse_squares.py',       # sparse (0.39% density in 16-bit domain)
    # Dim4 — number of variables
    'run_distr_sum_10pairs.py',          # 2 coupled vars
    'run_distr_exactly5_10b.py',         # 10 vars, combinatorial
    # Dim5 — variable width
    'run_distr_width_4b.py',             # 4-bit
    'run_distr_width_8b.py',             # 8-bit
    'run_distr_width_16b.py',            # 16-bit
    'run_distr_width_32b.py',            # 32-bit
    # Dim6 — real-world constraints taken from OpenTitan
    'run_distr_ot_aes_config.py',        # 5 vars, mixed constraints
    'run_distr_ot_aon_wkup.py',          # asymmetric branches
    'run_distr_ot_rv_timer_mul.py',      # multiplication
    'run_distr_ot_spi_freq.py',          # frequency ratio selection
    'run_distr_ot_usbdev_clk_ratio.py',  # clock ratio selection
]

# Benchmarks over array/queue rand variables. Run after the scalar ones, as they
# exercise a different sampler path (per-element hashing and write-back).
ARRAY_RUNNERS = [
    # Dim1 — constraint type
    'run_distr_array_basic.py',          # per-element range (anchor baseline)
    'run_distr_array_sum.py',            # sum over elements
    'run_distr_array_double_sum.py',     # sum with per-element scaling
    'run_distr_array_ordering_chain.py',  # ordering between neighbours
    # Dim2 — number of elements
    'run_distr_array_many_elem.py',      # 6 elements
    # Dim3 — element width
    'run_distr_array_wide_sum.py',       # 4-bit elements
    'run_distr_array_queue_wide.py',     # 8-bit elements, unbounded queue
    # Dim4 — array kind
    'run_distr_array_2d_range.py',       # 2-D, per-element range
    'run_distr_array_2d_sum.py',         # 2-D, sum
    'run_distr_array_assoc_sum.py',      # associative array
    # Dim5 — mixed with scalars
    'run_distr_array_mixed.py',          # array plus a scalar selector
]

RUNNERS = SCALAR_RUNNERS + ARRAY_RUNNERS


def clean_output_dir(output_dir):
    """Remove stale generated files so run_all always produces fresh artifacts."""
    if not output_dir.exists():
        return
    for path in output_dir.glob('build_*'):
        if path.is_dir():
            shutil.rmtree(path, ignore_errors=True)
    for pattern in ('*.svg', '*.rst', '*.jsonl'):
        for path in output_dir.glob(pattern):
            if path.is_file():
                path.unlink()


if __name__ == '__main__':
    num_iterations = parse_iterations()
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    clean_output_dir(OUTPUT_DIR)

    write_report_header(num_iterations)

    for runner in RUNNERS:
        cmd = [sys.executable, str(SCRIPTS_DIR / runner)]
        if num_iterations is not None:
            cmd.append(str(num_iterations))
        subprocess.run(cmd, check=True)

    write_summary_table()

    print('\nMerged report: ' + str(OUTPUT_DIR / 'verilator-distribution.rst'))
