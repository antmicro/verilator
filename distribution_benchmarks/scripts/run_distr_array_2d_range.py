import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_2d_range'

# 2x4 array of independent 2-bit elements, each in [0:2]: 3^8 = 6561 solutions.
ALL_SOLUTIONS = sorted(
    ' '.join(str(v) for v in vals) for vals in product(range(3), repeat=8)
)

assert len(ALL_SOLUTIONS) == 6561, f'Expected 6561 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
