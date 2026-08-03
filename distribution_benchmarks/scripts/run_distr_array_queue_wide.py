import sys
from itertools import product
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_queue_wide'

# Four 8-bit-stored queue elements, domain narrowed to [0:7] each: 8^4 = 4096 solutions.
ALL_SOLUTIONS = sorted(
    ' '.join(str(v) for v in vals) for vals in product(range(8), repeat=4)
)

assert len(ALL_SOLUTIONS) == 4096, f'Expected 4096 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
