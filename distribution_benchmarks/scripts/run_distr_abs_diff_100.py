import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_abs_diff_100'

# |x - y| == 50, x and y in [1:100]
# Group A (x > y): x in [51:100], y = x - 50
# Group B (y > x): x in [1:50],   y = x + 50
ALL_SOLUTIONS = []
for x in range(51, 101):
    y = x - 50
    ALL_SOLUTIONS.append(str(x) + ' ' + str(y))
for x in range(1, 51):
    y = x + 50
    ALL_SOLUTIONS.append(str(x) + ' ' + str(y))

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
