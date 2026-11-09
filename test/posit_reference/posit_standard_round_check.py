from fractions import Fraction
import subprocess
import sys


def pow2(exponent):
    return Fraction(1 << exponent, 1) if exponent >= 0 else Fraction(1, 1 << -exponent)


def standard_decode(raw, nbits):
    if raw == 0:
        return Fraction(0)
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
    fraction_bits = max(cursor + 1, 0)
    fraction_raw = raw & ((1 << fraction_bits) - 1) if fraction_bits else 0
    significand = Fraction(1 - 3 * sign, 1) + Fraction(fraction_raw, 1 << fraction_bits)
    scale = (1 - 2 * sign) * (4 * regime + exponent + sign)
    return significand * pow2(scale)


def to_encode_input(value):
    negative = value < 0
    magnitude = -value if negative else value
    numerator, denominator = magnitude.numerator, magnitude.denominator
    assert denominator & (denominator - 1) == 0
    return negative, numerator, -(denominator.bit_length() - 1)


cases = []
for positive_lower in range(1, 0x7fff):
    positive_upper = positive_lower + 1

    # Positive U1 is the 17-bit posit formed by appending 1 to U.
    positive_u1 = (positive_lower << 1) | 1
    positive_threshold = standard_decode(positive_u1, 17)
    expected_positive = positive_lower if (positive_lower & 1) == 0 else positive_upper
    sign, sig, scale = to_encode_input(positive_threshold)
    cases.append((sign, sig, scale, expected_positive,
                  f'positive U=0x{positive_lower:04x}', positive_threshold))

    # Negative lower numeric endpoint is -positive_upper. Build its literal
    # 16-bit U and append 1 exactly as Section 4.1 specifies.
    negative_u = (-positive_upper) & 0xffff
    negative_w = (-positive_lower) & 0xffff
    negative_u1 = ((negative_u << 1) | 1) & 0x1ffff
    negative_threshold = standard_decode(negative_u1, 17)
    expected_negative = negative_u if (negative_u & 1) == 0 else negative_w
    sign, sig, scale = to_encode_input(negative_threshold)
    cases.append((sign, sig, scale, expected_negative,
                  f'negative U=0x{negative_u:04x}', negative_threshold))

# Section 4.1 projects all nonzero magnitudes below minpos to signed minpos.
for value, expected, label in [
    (Fraction(1, 1 << 57), 0x0001, '+2^-57 under minpos'),
    (Fraction(-1, 1 << 57), 0xffff, '-2^-57 under minpos'),
]:
    sign, sig, scale = to_encode_input(value)
    cases.append((sign, sig, scale, expected, label, value))

payload = ''.join(f'{int(sign)} {sig} {scale}\n' for sign, sig, scale, *_ in cases)
actual = subprocess.run([sys.argv[1]], input=payload, text=True,
                        capture_output=True, check=True).stdout.splitlines()
assert len(actual) == len(cases), (len(actual), len(cases))
mismatches = []
for case, output in zip(cases, actual):
    sign, sig, scale, expected, label, exact_value = case
    got = int(output)
    if got != expected:
        mismatches.append((label, exact_value, expected, got))

print(f'positive U1 thresholds checked: {0x7fff - 1}')
print(f'negative U1 thresholds checked: {0x7fff - 1}')
print('signed under-minpos cases checked: 2')
print(f'total cases={len(cases)} mismatches={len(mismatches)}')
for mismatch in mismatches[:20]:
    print(mismatch)
raise SystemExit(bool(mismatches))
