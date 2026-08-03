import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_range_1M'

# Trivial large range: x inside {[0:999999]}, 1M solutions
# Variable: bit [19:0], domain [0:1048575]
# Canonical order: ascending integer value
ALL_SOLUTIONS = list(range(0, 1_000_000))

assert len(ALL_SOLUTIONS) == 1_000_000, f'Expected 1000000 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
