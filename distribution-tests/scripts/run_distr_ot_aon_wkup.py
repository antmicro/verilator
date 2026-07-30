import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_ot_aon_wkup'

# Adapted from OpenTitan aon_timer_base_vseq: wkup threshold/count constraint
# thold in {0}|[5:171], gap fixed to 5
# thold==0 -> count==0 (1 solution)
# thold!=0 -> count in [thold-5:thold] (6 solutions per thold value)
# Solutions: 1 + 167*6 = 1003
# Canonical order: (thold, count) lexicographic ascending
ALL_SOLUTIONS = sorted(
    (
        [(0, 0)] +
        [
            (thold, count)
            for thold in range(5, 172)
            for count in range(thold - 5, thold + 1)
        ]
    ),
    key=lambda t: t,
)
ALL_SOLUTIONS = [f'{thold} {count}' for thold, count in ALL_SOLUTIONS]

assert len(ALL_SOLUTIONS) == 1003, f'Expected 1003 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
