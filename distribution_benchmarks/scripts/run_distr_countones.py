import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_countones'

# x in [0:255] (8-bit), $countones(x) == 4  =>  C(8,4) = 70 solutions
# Enumerate all 8-bit values with exactly 4 bits set.
ALL_SOLUTIONS = [i for i in range(0, 256) if bin(i).count('1') == 4]

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
