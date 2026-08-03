import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_ot_usbdev_clk_ratio'

# Source: OpenTitan hw/ip/usbdev/dv/env/usbdev_env_cfg.sv
# AON clock frequency constrained relative to fixed USB clock (48_000 kHz):
#   aon_clk_freq_khz > 48_000 / 300  => > 160  => >= 161
#   aon_clk_freq_khz <= 48_000 / 48  => <= 1000
# Solutions: {161, 162, ..., 1000} = 840 solutions
# Variable: int unsigned (32-bit)
# Canonical order: ascending integer value
ALL_SOLUTIONS = list(range(161, 1001))

assert len(ALL_SOLUTIONS) == 840, f'Expected 840 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
