import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_mixed_9989'

# Mixed: arithmetic + bitwise implication, ~10k solutions
# base in [0:199], delta in [1:63]
# base + delta <= 200
# base >= 128 -> delta <= 35
# Solutions: 9989
# Canonical order: (base, delta) lexicographic ascending
ALL_SOLUTIONS = sorted(
    [
        f'{base} {delta}'
        for base in range(0, 200)
        for delta in range(1, 64)
        if base + delta <= 200 and (base < 128 or delta <= 35)
    ],
    key=lambda s: tuple(int(x) for x in s.split()),
)

assert len(ALL_SOLUTIONS) == 9989, f'Expected 9989 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
