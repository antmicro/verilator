import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_double_sum'

# Six 3-bit elements, each in [0:7]: arr[0]+arr[1]+arr[2]==10 (48 solutions)
# AND arr[3]+arr[4]+arr[5]==8 (42 solutions), independent clusters: 48*42 = 2016.
# Canonical order: (arr[0],...,arr[5]) lexicographic ascending.
ALL_SOLUTIONS = sorted(
    f'{a} {b} {c} {d} {e} {f}'
    for a, b, c in product(range(8), repeat=3)
    if a + b + c == 10
    for d, e, f in product(range(8), repeat=3)
    if d + e + f == 8
)

assert len(ALL_SOLUTIONS) == 2016, f'Expected 2016 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
