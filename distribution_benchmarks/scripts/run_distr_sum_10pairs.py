import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_sum_10pairs'

# x + y == 11, x and y in [1:10]
# Valid pairs: (1,10), (2,9), (3,8), (4,7), (5,6), (6,5), (7,4), (8,3), (9,2), (10,1)
ALL_SOLUTIONS = []
for x in range(1, 11):
    y = 11 - x
    ALL_SOLUTIONS.append(str(x) + ' ' + str(y))

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
