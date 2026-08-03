import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_sparse_squares'

# All perfect squares in 16-bit domain: n² for n in [0:255], n²<=65535
# Solutions: 256 values {0, 1, 4, 9, ..., 65025}
# Domain: 65536 -- density: 0.39% (sparse)
ALL_SOLUTIONS = [i * i for i in range(0, 256) if i * i <= 65535]

assert len(ALL_SOLUTIONS) == 256, f'Expected 256 solutions, got {len(ALL_SOLUTIONS)}'
assert ALL_SOLUTIONS[-1] == 65025

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
