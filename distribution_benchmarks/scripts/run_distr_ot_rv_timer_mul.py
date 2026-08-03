import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_ot_rv_timer_mul'

# Source: OpenTitan hw/ip/rv_timer/dv/env/seq_lib/rv_timer_random_vseq.sv
# Multiplication constraint: ticks * (prescale + 1) <= 1000
# prescale in [0:9], ticks in [1:1000]
# Solutions: all (prescale, ticks) pairs satisfying the constraint
#   prescale=0: 1000, prescale=1: 500, ..., prescale=9: 100
#   total: 2927 solutions
ALL_SOLUTIONS = sorted(
    [
        f'{prescale} {ticks}'
        for prescale in range(10)
        for ticks in range(1, 1001)
        if ticks * (prescale + 1) <= 1000
    ],
    key=lambda s: tuple(int(x) for x in s.split()),
)

assert len(ALL_SOLUTIONS) == 2927, f'Expected 2927 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
