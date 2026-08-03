import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_quadratic_100'

# x in [1:1000], x*x <= 10000  =>  solutions are {1, 2, ..., 100}
ALL_SOLUTIONS = list(range(1, 101))

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
