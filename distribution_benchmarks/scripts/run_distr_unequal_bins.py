import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_unequal_bins'

# Unequal bins: mode=0 -> x in [0:99] (100 solutions)
#               mode=1 -> x in [0:9999] (10000 solutions)
# Total: 10100 solutions, bin ratio 100x
ALL_SOLUTIONS = sorted(
    [f'0 {x}' for x in range(0, 100)] +
    [f'1 {x}' for x in range(0, 10000)],
    key=lambda s: tuple(int(v) for v in s.split()),
)

assert len(ALL_SOLUTIONS) == 10100, f'Expected 10100 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
