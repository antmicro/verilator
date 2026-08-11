import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment_large, parse_iterations

MODULE_NAME = 'distr_large_mod'

NUM_SOLUTIONS = 261632

if __name__ == '__main__':
    run_experiment_large(MODULE_NAME, NUM_SOLUTIONS, parse_iterations())
