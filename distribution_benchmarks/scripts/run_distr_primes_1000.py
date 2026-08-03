import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from runner import run_experiment, parse_iterations

MODULE_NAME = 'distr_primes_1000'

# Set membership: the first 1000 prime numbers (2, 3, 5, ..., 7919)
# Variable: bit [15:0], domain [0:65535]
# Constraint encoded as OR of 1000 equality atoms in SMT-LIB
# Solutions: exactly 1000
# Canonical order: ascending integer value


def _first_1000_primes():
    is_p = [True] * 8000
    is_p[0] = is_p[1] = False
    for i in range(2, int(len(is_p) ** 0.5) + 1):
        if is_p[i]:
            for j in range(i * i, len(is_p), i):
                is_p[j] = False
    return [i for i in range(len(is_p)) if is_p[i]][:1000]


ALL_SOLUTIONS = _first_1000_primes()

assert len(ALL_SOLUTIONS) == 1000, f'Expected 1000 solutions, got {len(ALL_SOLUTIONS)}'
assert ALL_SOLUTIONS[-1] == 7919, f'Expected last prime 7919, got {ALL_SOLUTIONS[-1]}'

if __name__ == '__main__':
    run_experiment(MODULE_NAME, ALL_SOLUTIONS, parse_iterations())
