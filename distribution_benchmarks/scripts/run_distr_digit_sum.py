import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_digit_sum'

# x in [1000:9999], digit sum of x == 8
# Enumerate all 4-digit numbers whose digits sum to 8.
ALL_SOLUTIONS = []
for x in range(1000, 10000):
    d1 = x // 1000
    d2 = (x // 100) % 10
    d3 = (x // 10) % 10
    d4 = x % 10
    if d1 + d2 + d3 + d4 == 8:
        ALL_SOLUTIONS.append(x)

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
