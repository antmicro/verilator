import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_sum'

# 3-element array, each in [0:7], arr[0]+arr[1]+arr[2] == 10: 48 solutions.
ALL_SOLUTIONS = sorted(
    f'{a} {b} {c}' for a, b, c in product(range(8), repeat=3) if a + b + c == 10
)

assert len(ALL_SOLUTIONS) == 48, f'Expected 48 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
