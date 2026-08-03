import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_modulo_100k'

# Large modular constraint: x % 7 == 0, x inside {[1:700000]}
# Variable: bit [19:0], domain [0:1048575]
# Solutions: {7, 14, 21, ..., 700000} = 100000
ALL_SOLUTIONS = list(range(7, 700001, 7))

assert len(ALL_SOLUTIONS) == 100000, f'Expected 100000 solutions, got {len(ALL_SOLUTIONS)}'
assert ALL_SOLUTIONS[-1] == 700000

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
