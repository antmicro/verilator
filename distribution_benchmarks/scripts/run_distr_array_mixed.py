import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_mixed'

# 2-element array (each in [0:7]) + a scalar `sel` bit, linked by:
#   sel==1 -> arr[0] >  arr[1]
#   sel==0 -> arr[0] <= arr[1]
# sel is fully determined by (arr[0],arr[1]): 8*8 = 64 solutions total.
def _enumerate():
    solutions = []
    for arr0, arr1 in product(range(8), repeat=2):
        sel = 1 if arr0 > arr1 else 0
        solutions.append(f'{sel} {arr0} {arr1}')
    return sorted(solutions)

ALL_SOLUTIONS = _enumerate()

assert len(ALL_SOLUTIONS) == 64, f'Expected 64 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
