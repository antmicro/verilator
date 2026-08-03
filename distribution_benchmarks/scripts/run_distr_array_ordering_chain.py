import sys
from itertools import combinations
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_array_ordering_chain'

# Four 3-bit elements, each in [0:7], strictly increasing: C(8,4) = 70 solutions.
# Canonical order: (arr[0],arr[1],arr[2],arr[3]) lexicographic ascending.
ALL_SOLUTIONS = sorted(' '.join(str(v) for v in combo) for combo in combinations(range(8), 4))

assert len(ALL_SOLUTIONS) == 70, f'Expected 70 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
