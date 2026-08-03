import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_2d_sum'

# 2x2 matrix of 3-bit elements, each in [0:7], full sum == 10: 246 solutions.
ALL_SOLUTIONS = sorted(
    ' '.join(str(v) for v in vals) for vals in product(range(8), repeat=4) if sum(vals) == 10
)

assert len(ALL_SOLUTIONS) == 246, f'Expected 246 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
