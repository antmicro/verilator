import sys
from itertools import combinations
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_exactly5_10b'

# 10 independent 1-bit variables, exactly 5 set: C(10,5) = 252 solutions
# Canonical order: (b0,b1,...,b9) lexicographic ascending
def _enumerate():
    solutions = []
    for positions in combinations(range(10), 5):
        bits = [0] * 10
        for p in positions:
            bits[p] = 1
        solutions.append(' '.join(str(b) for b in bits))
    return sorted(solutions)

ALL_SOLUTIONS = _enumerate()

assert len(ALL_SOLUTIONS) == 252, f'Expected 252 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
