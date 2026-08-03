import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_many_elem'

# Six independent 3-bit elements, each in [0:4]: 5^6 = 15625 solutions.
# Canonical order: (arr[0],...,arr[5]) lexicographic ascending.
ALL_SOLUTIONS = sorted(
    ' '.join(str(v) for v in vals) for vals in product(range(5), repeat=6)
)

assert len(ALL_SOLUTIONS) == 15625, f'Expected 15625 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
