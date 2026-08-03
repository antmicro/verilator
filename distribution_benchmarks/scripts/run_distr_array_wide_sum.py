import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_wide_sum'

# Four 4-bit elements, each in [0:15], arr[0]+arr[1]+arr[2]+arr[3] == 30: 2736 solutions.
ALL_SOLUTIONS = sorted(
    f'{a} {b} {c} {d}'
    for a, b, c, d in product(range(16), repeat=4)
    if a + b + c + d == 30
)

assert len(ALL_SOLUTIONS) == 2736, f'Expected 2736 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
