from fractions import Fraction


def decode_positive(raw, nbits):
    if raw == 0:
        return Fraction(0)
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
    return (1 + Fraction(fraw, 1 << fbits)) * (Fraction(1 << (4 * regime + exponent), 1) if 4 * regime + exponent >= 0 else Fraction(1, 1 << -(4 * regime + exponent)))


different = []
for lower in range(0x0001, 0x7fff):
    lo = decode_positive(lower, 16)
    hi = decode_positive(lower + 1, 16)
    v = decode_positive((lower << 1) | 1, 17)
    arithmetic_midpoint = (lo + hi) / 2
    if v != arithmetic_midpoint:
        different.append((lower, lo, hi, v, arithmetic_midpoint))

print(f'adjacent positive intervals scanned: {0x7fff - 1}')
print(f'intervals where 17-bit U1 differs from arithmetic midpoint: {len(different)}')
for lower, lo, hi, v, mid in different[:12]:
    print(f'lower=0x{lower:04x} upper=0x{lower+1:04x} v={v} midpoint={mid} v_side={"low" if v < mid else "high"}')
