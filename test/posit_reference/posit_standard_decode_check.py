from fractions import Fraction
import sys


def pow2(exponent):
    return Fraction(1 << exponent, 1) if exponent >= 0 else Fraction(1, 1 << -exponent)


def standard_decode(raw):
    if raw == 0:
        return Fraction(0), False
    if raw == 0x8000:
        return None, True

    sign = (raw >> 15) & 1
    cursor = 14
    regime_bit = (raw >> cursor) & 1
    run = 0
    while cursor >= 0 and ((raw >> cursor) & 1) == regime_bit:
        run += 1
        cursor -= 1
    regime = run - 1 if regime_bit else -run
    if cursor >= 0:  # Consume the regime terminator.
        cursor -= 1

    exponent = 0
    for _ in range(2):
        exponent = (exponent << 1) | (((raw >> cursor) & 1) if cursor >= 0 else 0)
        if cursor >= 0:
            cursor -= 1

    fraction_bits = max(cursor + 1, 0)
    fraction_raw = raw & ((1 << fraction_bits) - 1) if fraction_bits else 0
    fraction = Fraction(fraction_raw, 1 << fraction_bits)
    significand = Fraction(1 - 3 * sign, 1) + fraction
    scale = (1 - 2 * sign) * (4 * regime + exponent + sign)
    return significand * pow2(scale), False


def decoded_value(negative, significand, scale):
    value = Fraction(significand, 1) * pow2(scale)
    return -value if negative else value


mismatches = []
checked = 0
for line in open(sys.argv[1] if len(sys.argv) > 1 else '/tmp/posit_decode_dump.txt'):
    raw_s, nar_s, neg_s, sig_s, scale_s = line.split()
    raw, is_nar = int(raw_s), bool(int(nar_s))
    neg, sig, scale = bool(int(neg_s)), int(sig_s), int(scale_s)
    ref_value, ref_nar = standard_decode(raw)
    if is_nar != ref_nar:
        mismatches.append((raw, 'NaR classification', ref_nar, is_nar))
    elif not is_nar:
        checked += 1
        got = decoded_value(neg, sig, scale)
        if got != ref_value:
            mismatches.append((raw, 'value', ref_value, got))

print(f'checked={checked} mismatches={len(mismatches)}')
for item in mismatches[:20]:
    print(item)
for raw in (0x0000, 0x0001, 0x4000, 0x7fff, 0x8000, 0x8001, 0xc000, 0xffff):
    print(f'{raw:04x}: {standard_decode(raw)}')
raise SystemExit(bool(mismatches))
