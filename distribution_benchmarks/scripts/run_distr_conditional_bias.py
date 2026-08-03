import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_conditional_bias'

# mode=0 -> value in {0, 1}      (2 pairs)
# mode=1 -> value in {2, 3, 4, 5} (4 pairs)
# mode=2 -> value in {6, 7, 8, 9} (4 pairs)
# Total: 10 solutions
ALL_SOLUTIONS = []
for value in range(0, 2):
    ALL_SOLUTIONS.append('0 ' + str(value))
for value in range(2, 6):
    ALL_SOLUTIONS.append('1 ' + str(value))
for value in range(6, 10):
    ALL_SOLUTIONS.append('2 ' + str(value))

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
