"""Independent Posit Standard Quire oracle; requires the posit_quire_cli adapter."""
from bisect import bisect_left
from fractions import Fraction
import random
import subprocess
import sys

MASK = (1 << 256) - 1
NAR = 0x8000
Q_NAR = 1 << 255
Q_MIN = -Q_NAR + 1
Q_MAX = Q_NAR - 1
UNIT = 1 << 112


def pdecode(raw, nbits=16):
    if raw == 0:
        return Fraction(0)
    if nbits == 16 and raw == NAR:
        return None
    cursor = nbits - 2
    sign = (raw >> (nbits - 1)) & 1
    regime_bit = (raw >> cursor) & 1
    run = 0
    while cursor >= 0 and ((raw >> cursor) & 1) == regime_bit:
        cursor -= 1
        run += 1
    regime = run - 1 if regime_bit else -run
    if cursor >= 0:
        cursor -= 1
    exponent = 0
    for _ in range(2):
        exponent = (exponent << 1) | ((raw >> cursor) & 1 if cursor >= 0 else 0)
        if cursor >= 0:
            cursor -= 1
    fbits = cursor + 1
    fraction = raw & ((1 << fbits) - 1) if fbits else 0
    significand = Fraction(1 - 3 * sign, 1) + Fraction(fraction, 1 << fbits)
    value = significand
    power = (1 - 2 * sign) * (4 * regime + exponent + sign)
    value *= Fraction(1 << power, 1) if power >= 0 else Fraction(1, 1 << -power)
    return value


POSITIVE = [pdecode(code) for code in range(0x8000)]
EXACT = {value: code for code, value in enumerate(POSITIVE)}
MINPOS = Fraction(1, 1 << 56)
MAXPOS = Fraction(1 << 56)


def pencode(value):
    if value == 0:
        return 0
    negative = value < 0
    mag = -value if negative else value
    if mag < MINPOS:
        return 0xffff if negative else 0x0001
    if mag >= MAXPOS:
        return 0x8001 if negative else 0x7fff
    if mag in EXACT:
        code = EXACT[mag]
    else:
        high = bisect_left(POSITIVE, mag)
        low = high - 1
        threshold = pdecode((low << 1) | 1, 17)
        if mag < threshold or (mag == threshold and low % 2 == 0):
            code = low
        else:
            code = high
    if not negative:
        return code
    if mag in EXACT:
        return (-code) & 0xffff
    upper = bisect_left(POSITIVE, mag)
    lower = upper - 1
    low_raw = (-upper) & 0xffff
    high_raw = (-lower) & 0xffff
    boundary = pdecode(((low_raw << 1) | 1) & 0x1ffff, 17)
    return low_raw if value < boundary or (value == boundary and low_raw % 2 == 0) else high_raw


def units_for(value):
    scaled = value * UNIT
    assert scaled.denominator == 1
    return scaled.numerator


def qwords(value):
    raw = value & MASK
    return ' '.join(f'{(raw >> (32 * i)) & 0xffffffff:08x}' for i in range(7, -1, -1))


operations = []
expected = []


def add(op, q, a=0, b=0):
    operations.append(f'{op} {qwords(q)} {a:04x} {b:04x}' if op != 'R'
                      else f'R {qwords(q)}')
    if op == 'R':
        result = NAR if q == -Q_NAR else pencode(Fraction(q, UNIT))
        expected.append(result)
        return result
    av, bv = pdecode(a), pdecode(b)
    if q == -Q_NAR or av is None or bv is None:
        result = -Q_NAR
    else:
        product = units_for(av * bv)
        result = q + product if op == 'A' else q - product
        if result < Q_MIN or result > Q_MAX:
            result = -Q_NAR
    expected.append(result)
    return result


# Sign combinations, zero, exact accumulation/cancellation, and boundary operands.
for a, b in [(0x4000, 0x4800), (0x4000, 0xc000), (0xc000, 0xc000),
             (0, 0x7fff), (1, 0x7fff), (0x7fff, 0x7fff), (0xffff, 0x7fff)]:
    add('A', 0, a, b)
add('A', units_for(Fraction(4)), 0xc000, 0x5000)  # 4 + (-1*4) = 0
add('S', units_for(Fraction(4)), 0x4000, 0x4000)  # 4 - 1 = 3
add('A', -Q_NAR, 0x4000, 0x4000)
add('A', 0, NAR, 0x4000)
add('A', 0, 0x4000, NAR)
add('A', Q_MAX, 0x4000, 0x4000)
add('S', Q_MIN, 0x4000, 0x4000)
# The minimum-posit product is one Quire unit, so this reaches the reserved
# -2^255 NaR encoding exactly from the most-negative finite value.
add('S', -Q_NAR + 1, 0x0001, 0x0001)
add('S', units_for(Fraction(1)), 0, 0x7fff)  # zero product preserves q
add('R', -Q_NAR)

# Chain operations through exact Python accumulator values and cancel to zero.
chain = add('A', 0, 0x4800, 0x4800)          # +4
chain = add('A', chain, 0x4000, 0x4000)      # +1 -> 5
chain = add('S', chain, 0x4800, 0x4800)      # -4 -> 1
chain = add('S', chain, 0x4000, 0x4000)      # -1 -> 0

# Cover each finite encoding's exact decoder/product path with multiplication by 1.
for raw in range(0x10000):
    if raw != NAR:
        add('A', 0, raw, 0x4000)

rng = random.Random(20261119)
for _ in range(4000):
    a, b = rng.randrange(0x10000), rng.randrange(0x10000)
    q = rng.randrange(-Q_NAR + 1, Q_NAR)
    add(rng.choice('AS'), q, a, b)

# Every positive U1 boundary and its negative counterpart, plus adjacent Quire units.
for low in range(1, 0x7fff):
    boundary = units_for(pdecode((low << 1) | 1, 17))
    for n in {boundary - 1, boundary, boundary + 1,
              -boundary - 1, -boundary, -boundary + 1}:
        add('R', n)

# Include exact Posit values, under-minpos, over-maxpos, and deterministic arbitrary Q values.
for raw in [0, 1, 0xffff, 0x4000, 0xc000, 0x7fff, 0x8001, 0x8000]:
    value = pdecode(raw)
    if value is not None:
        add('R', units_for(value))
add('R', 1)
add('R', -1)
add('R', units_for(MAXPOS) + 1)
add('R', -(units_for(MAXPOS) + 1))
for _ in range(500):
    add('R', rng.randrange(-Q_NAR + 1, Q_NAR))

proc = subprocess.run([sys.argv[1]], input='\n'.join(operations) + '\n', text=True,
                      capture_output=True, check=True)
outputs = proc.stdout.splitlines()
assert len(outputs) == len(expected), (len(outputs), len(expected), proc.stderr)
errors = []
for index, (line, want, op) in enumerate(zip(outputs, expected, operations)):
    got = int(line, 16) if op[0] != 'R' else int(line)
    if got != (want & MASK if op[0] != 'R' else want):
        errors.append((index, op, want, got))
        if len(errors) == 20:
            break
print(f'operations={len(operations)} (deterministic accumulation + QROUND/U1 boundaries)')
print(f'mismatches={len(errors)}')
for error in errors:
    print(error)
raise SystemExit(bool(errors))
