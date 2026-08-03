import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_implication'

# Constraint: a -> b  (if a==1 then b must be 1)
# Valid pairs (a, b): (0,0), (0,1), (1,1)
ALL_SOLUTIONS = ['0 0', '0 1', '1 1']

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
