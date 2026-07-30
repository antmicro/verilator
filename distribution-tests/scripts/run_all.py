import sys
import shutil
import subprocess
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import parse_iterations, write_report_header, write_summary_table

SCRIPTS_DIR = Path(__file__).resolve().parent
OUTPUT_DIR  = Path(__file__).resolve().parent.parent / 'output'

RUNNERS = [
    'run_distr_array_mixed.py',   # array + scalar, conditional constraint (64 solutions)
    'run_distr_ot_aon_wkup.py',   # OpenTitan aon_timer_base_vseq wkup threshold/count (1003 solutions)
    'run_distr_countones.py',     # 8-bit value, exactly 4 bits set (70 solutions)
]


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
