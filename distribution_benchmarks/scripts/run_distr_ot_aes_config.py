import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_ot_aes_config'

# Source: OpenTitan hw/ip/aes/dv/env/aes_message_item.sv
#         hw/ip/aes/rtl/aes_pkg.sv (encoding constants)
#
# Variables: has_error, op (2-bit), key_len (3-bit), mode (6-bit), reseed_rate (3-bit)
# Encoding constants (from aes_pkg.sv):
#   AES_ENC=0b01,  AES_DEC=0b10
#   AES_128=0b001, AES_192=0b010, AES_256=0b100
#   AES_ECB=1,     AES_CBC=2,   AES_CFB=4,  AES_OFB=8,  AES_CTR=16, AES_GCM=32
#   AES_NONE=63  (6'b111111, illegal sentinel used in error mode)
#   PER_1=0b001,   PER_64=0b010, PER_8K=0b100
#
# Solution count:
#   has_error=0: 2 ops x 3 key_lens x 6 modes x 3 reseed_rates = 108
#   has_error=1: 2 ops x 5 key_lens x 1 mode  x 5 reseed_rates =  50
#   Total: 158
#

VALID_OPS       = [0b01, 0b10]            # AES_ENC, AES_DEC
VALID_KEY_LENS  = [0b001, 0b010, 0b100]   # AES_128, AES_192, AES_256
VALID_MODES     = [1, 2, 4, 8, 16, 32]   # ECB, CBC, CFB, OFB, CTR, GCM (one-hot)
VALID_RESEEDS   = [0b001, 0b010, 0b100]   # PER_1, PER_64, PER_8K

INVALID_OPS     = [0b00, 0b11]
INVALID_KEY_LENS = [0b000, 0b011, 0b101, 0b110, 0b111]
AES_NONE        = 0b111111               # 63
INVALID_RESEEDS = [0b000, 0b011, 0b101, 0b110, 0b111]

_normal = [
    f'0 {op} {kl} {mode} {rr}'
    for op   in VALID_OPS
    for kl   in VALID_KEY_LENS
    for mode in VALID_MODES
    for rr   in VALID_RESEEDS
]

_error = [
    f'1 {op} {kl} {AES_NONE} {rr}'
    for op in INVALID_OPS
    for kl in INVALID_KEY_LENS
    for rr in INVALID_RESEEDS
]

ALL_SOLUTIONS = sorted(
    _normal + _error,
    key=lambda s: tuple(int(x) for x in s.split())
)

assert len(ALL_SOLUTIONS) == 158, f'Expected 158 solutions, got {len(ALL_SOLUTIONS)}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
