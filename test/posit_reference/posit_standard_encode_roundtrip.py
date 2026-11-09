from fractions import Fraction
import subprocess
import sys

def p2(e): return Fraction(1 << e, 1) if e >= 0 else Fraction(1, 1 << -e)

def std_decode(raw):
    if raw == 0: return Fraction(0)
    if raw == 0x8000: return None
    s = (raw >> 15) & 1
    cursor = 14
    rb = (raw >> cursor) & 1
    run = 0
    while cursor >= 0 and ((raw >> cursor) & 1) == rb:
        run += 1; cursor -= 1
    regime = run - 1 if rb else -run
    if cursor >= 0: cursor -= 1
    exponent = 0
    for _ in range(2):
        exponent = (exponent << 1) | (((raw >> cursor) & 1) if cursor >= 0 else 0)
        if cursor >= 0: cursor -= 1
    fbits = max(cursor + 1, 0)
    fraw = raw & ((1 << fbits) - 1) if fbits else 0
    significand = Fraction(1 - 3*s, 1) + Fraction(fraw, 1 << fbits)
    scale = (1 - 2*s) * (4 * regime + exponent + s)
    return significand * p2(scale)

cases = []
for raw in range(0x10000):
    value = std_decode(raw)
    if value is None: continue
    neg = value < 0
    mag = -value if neg else value
    assert mag.denominator & (mag.denominator - 1) == 0
    scale = -(mag.denominator.bit_length() - 1)
    cases.append((neg, mag.numerator, scale, raw))

payload = ''.join(f'{int(n)} {s} {e}\n' for n, s, e, _ in cases)
proc = subprocess.run([sys.argv[1]], input=payload, text=True,
                      capture_output=True, check=True)
outs = [int(x) for x in proc.stdout.splitlines()]
mismatches = [(c, got) for c, got in zip(cases, outs) if c[3] != got]
print(f'exact standard-derived finite values tested: {len(cases)}')
print(f'mismatches: {len(mismatches)}')
for case, got in mismatches[:20]: print(case, got)
raise SystemExit(bool(mismatches))
