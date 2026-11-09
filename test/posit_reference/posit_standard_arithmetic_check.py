from bisect import bisect_left
from fractions import Fraction
import random
import subprocess
import sys


NAR = 0x8000
MAXPOS = 0x7fff
MINPOS_VALUE = Fraction(1, 1 << 56)
MAXPOS_VALUE = Fraction(1 << 56, 1)


def pow2(exponent):
    return Fraction(1 << exponent, 1) if exponent >= 0 else Fraction(1, 1 << -exponent)


def standard_decode(raw, nbits=16):
    if raw == 0:
        return Fraction(0)
    if nbits == 16 and raw == NAR:
        return None
    sign = (raw >> (nbits - 1)) & 1
    cursor = nbits - 2
    regime_bit = (raw >> cursor) & 1
    run = 0
    while cursor >= 0 and ((raw >> cursor) & 1) == regime_bit:
        run += 1
        cursor -= 1
    regime = run - 1 if regime_bit else -run
    if cursor >= 0:
        cursor -= 1
    exponent = 0
    for _ in range(2):
        exponent = (exponent << 1) | (((raw >> cursor) & 1) if cursor >= 0 else 0)
        if cursor >= 0:
            cursor -= 1
    fbits = max(cursor + 1, 0)
    fraw = raw & ((1 << fbits) - 1) if fbits else 0
    significand = Fraction(1 - 3 * sign, 1) + Fraction(fraw, 1 << fbits)
    scale = (1 - 2 * sign) * (4 * regime + exponent + sign)
    return significand * pow2(scale)


positive_values = [standard_decode(raw) for raw in range(MAXPOS + 1)]
exact_code = {value: raw for raw, value in enumerate(positive_values)}
threshold = {raw: standard_decode((raw << 1) | 1, 17) for raw in range(1, MAXPOS)}
negative_threshold = {}
for positive_lower in range(1, MAXPOS):
    negative_lower = (-positive_lower - 1) & 0xffff
    u1 = ((negative_lower << 1) | 1) & 0x1ffff
    negative_threshold[positive_lower] = standard_decode(u1, 17)


def standard_encode(value):
    if value == 0:
        return 0
    negative = value < 0
    magnitude = -value if negative else value
    if magnitude < MINPOS_VALUE:
        return 0xffff if negative else 0x0001
    elif magnitude > MAXPOS_VALUE:
        return 0x8001 if negative else MAXPOS
    elif magnitude in exact_code:
        code = exact_code[magnitude]
        return ((-code) & 0xffff) if negative else code

    upper = bisect_left(positive_values, magnitude)
    lower = upper - 1
    if not negative:
        v = threshold[lower]
        return lower if value < v or (value == v and (lower & 1) == 0) else upper

    # For negative x, U is the lower numeric encoding, -upper. Apply Section
    # 4.1 directly to U and its 17-bit U1 extension.
    u = (-upper) & 0xffff
    w = (-lower) & 0xffff
    v = negative_threshold[lower]
    return u if value < v or (value == v and (u & 1) == 0) else w


def expected(op, lhs, rhs):
    a = standard_decode(lhs)
    b = standard_decode(rhs)
    if a is None or b is None:
        return NAR
    if op == 'A':
        value = a + b
    elif op == 'S':
        value = a - b
    elif op == 'M':
        value = a * b
    else:
        if b == 0:
            return NAR
        value = a / b
    return standard_encode(value)


cases = []
# Identity operations cover every one of the 65,536 possible raw inputs,
# including NaR, through each arithmetic entry point.
for raw in range(0x10000):
    cases.extend([('A', raw, 0), ('S', raw, 0), ('M', raw, 0x4000), ('D', raw, 0x4000)])

# Deterministic broad pair sampling; includes arbitrary signs, NaR, zero and
# exponent/regime boundaries. This is sampling, not exhaustive pair testing.
rng = random.Random(20261109)
for op in 'ASMD':
    for _ in range(2500):
        cases.append((op, rng.randrange(0x10000), rng.randrange(0x10000)))

payload = ''.join(f'{op} {lhs} {rhs}\n' for op, lhs, rhs in cases)
proc = subprocess.run([sys.argv[1]], input=payload, text=True,
                      capture_output=True, check=True)
outputs = proc.stdout.splitlines()
assert len(outputs) == len(cases), (len(outputs), len(cases))
mismatches = []
for case, output in zip(cases, outputs):
    op, lhs, rhs = case
    want = expected(op, lhs, rhs)
    got = int(output)
    if want != got:
        mismatches.append((op, lhs, rhs, want, got))
        if len(mismatches) >= 20:
            break

print(f'cases={len(cases)} (identity coverage=262144, deterministic pairs=10000)')
print(f'mismatches={len(mismatches)}')
for item in mismatches:
    print(item)
raise SystemExit(bool(mismatches))
