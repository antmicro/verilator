import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_assoc_sum'

# Associative array, 3 populated keys, each in [0:7]: aa[10]+aa[20]+aa[30]==15 -> 28 solutions.
# Canonical order: (aa[10],aa[20],aa[30]) lexicographic ascending.
ALL_SOLUTIONS = sorted(
    f'{a} {b} {c}' for a, b, c in product(range(8), repeat=3) if a + b + c == 15
)

assert len(ALL_SOLUTIONS) == 28, f'Expected 28 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
