import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_modulo_1000'

# Pure modulo constraint: x % 5 == 0, x inside {[1:5000]}
# Variable: bit [15:0], domain [0:65535]
# Solutions: {5, 10, 15, ..., 5000} = 1000
ALL_SOLUTIONS = list(range(5, 5001, 5))

assert len(ALL_SOLUTIONS) == 1000, f'Expected 1000 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
