import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_ot_spi_freq'

# Source: OpenTitan hw/ip/spi_device/dv/env/seq_lib/spi_device_base_vseq.sv
# spi_freq_faster=0: core_spi_freq_ratio in {1..8} -> 8 solutions
# spi_freq_faster=1: core_spi_freq_ratio in {1..4} -> 4 solutions
# Canonical order: sorted by (spi_freq_faster, core_spi_freq_ratio)
ALL_SOLUTIONS = [
    f'{faster} {ratio}'
    for faster in range(2)
    for ratio in range(1, 9)
    if not (faster == 1 and ratio > 4)
]

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
