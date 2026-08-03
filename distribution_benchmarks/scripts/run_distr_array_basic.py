import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_basic'

# 3-element array, each element independently in [0:4]: 5 x 5 x 5 = 125 solutions.
# Canonical order: (arr[0],arr[1],arr[2]) lexicographic ascending.
ALL_SOLUTIONS = sorted(
    f'{a} {b} {c}' for a, b, c in product(range(5), repeat=3)
)

assert len(ALL_SOLUTIONS) == 125, f'Expected 125 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
