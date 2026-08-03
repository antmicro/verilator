import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_width_32b'

# Width isolation: 32-bit variable (int unsigned), x inside {[0:9]}, 10 solutions
# Part of the width series: 4b / 8b / 16b / 32b, same constraint, same |M|
ALL_SOLUTIONS = list(range(0, 10))

assert len(ALL_SOLUTIONS) == 10, f'Expected 10 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
