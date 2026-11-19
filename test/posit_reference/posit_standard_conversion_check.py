"""Exact conversion checks from Posit Standard §§4.1, 5.2, 6.4 and 6.5."""
from fractions import Fraction
import random
import subprocess
import sys


def pow2(e):
    return Fraction(1 << e, 1) if e >= 0 else Fraction(1, 1 << -e)


def decode(raw, nbits=16):
    if raw == 0:
        return Fraction(0)
    sign = (raw >> (nbits - 1)) & 1
    cursor = nbits - 2
    rb = (raw >> cursor) & 1
    run = 0
    while cursor >= 0 and ((raw >> cursor) & 1) == rb:
        run += 1
        cursor -= 1
    k = run - 1 if rb else -run
    if cursor >= 0:
        cursor -= 1
    exponent = 0
    for _ in range(2):
        exponent = (exponent << 1) | ((raw >> cursor) & 1 if cursor >= 0 else 0)
        cursor -= cursor >= 0
    fbits = max(cursor + 1, 0)
    fraw = raw & ((1 << fbits) - 1) if fbits else 0
    sig = Fraction(1 - 3 * sign, 1) + Fraction(fraw, 1 << fbits)
    scale = (1 - 2 * sign) * (4 * k + exponent + sign)
    return sig * pow2(scale)


def as_binary32(value):
    sign = value < 0
    value = abs(value)
    if value == 0:
        return 0
    n, d = value.numerator, value.denominator
    exponent = n.bit_length() - d.bit_length()
    if value < pow2(exponent):
        exponent -= 1
    scaled = value / pow2(exponent - 23)
    significand = scaled.numerator // scaled.denominator
    assert scaled.denominator == 1 and (1 << 23) <= significand < (1 << 24)
    return (int(sign) << 31) | ((exponent + 127) << 23) | (significand & 0x7fffff)


cases = []
for u in range(1, 0x7fff):
    upper = u + 1
    tpos = decode((u << 1) | 1, 17)
    cases.append((as_binary32(tpos), u if not (u & 1) else upper,
                  f"positive U1 0x{u:04x}"))
    uneg = (-upper) & 0xffff
    wneg = (-u) & 0xffff
    tneg = decode(((uneg << 1) | 1) & 0x1ffff, 17)
    cases.append((as_binary32(tneg), uneg if not (uneg & 1) else wneg,
                  f"negative U1 U=0x{uneg:04x}"))

specials = [
    (0x00000000, 0x0000, "+0"), (0x80000000, 0x0000, "-0"),
    (0x7f800000, 0x8000, "+infinity"), (0xff800000, 0x8000, "-infinity"),
    (0x7fc00001, 0x8000, "quiet NaN"), (0x7f800001, 0x8000, "signaling NaN"),
    (0x00000001, 0x0001, "minimum subnormal"), (0x007fffff, 0x0001, "maximum subnormal"),
    (0x00800000, 0x0001, "minimum normal below minpos"),
    (0x23800000, 0x0001, "minpos"), (0xa3800000, 0xffff, "negative minpos"),
    (0x5b800000, 0x7fff, "maxpos"), (0xdb800000, 0x8001, "negative maxpos"),
    (0x7f7fffff, 0x7fff, "maximum finite binary32"),
]
cases.extend(specials)
payload = "".join(f"F {bits:08x}\n" for bits, _, _ in cases)
outputs = subprocess.run([sys.argv[1]], input=payload, text=True,
                         capture_output=True, check=True).stdout.splitlines()
assert len(outputs) == len(cases)
mismatches = [(name, f"{expected:04x}", f"{int(got):04x}")
              for (_, expected, name), got in zip(cases, outputs)
              if expected != int(got)]
print(f"positive binary32 U1 thresholds: 32766")
print(f"negative binary32 U1 thresholds: 32766")
print(f"special binary32 cases: {len(specials)}")
print(f"total={len(cases)} mismatches={len(mismatches)}")
for mismatch in mismatches[:20]:
    print(mismatch)

posit_inputs = list(range(0x10000))
posit_payload = "".join(f"P {bits:04x}\n" for bits in posit_inputs)
posit_outputs = subprocess.run([sys.argv[1]], input=posit_payload, text=True,
                               capture_output=True, check=True).stdout.splitlines()
assert len(posit_outputs) == len(posit_inputs)
integer_mismatches = []
float_mismatches = []
for bits, line in zip(posit_inputs, posit_outputs):
    integer_bits, float_bits = map(int, line.split())
    if bits == 0x8000:
        expected_integer, expected_float = 0x80000000, 0x7fc00000
    else:
        exact = decode(bits)
        rounded = round(exact)
        expected_integer = (0x80000000 if rounded < -(1 << 31) or rounded > (1 << 31) - 1
                            else rounded & 0xffffffff)
        expected_float = as_binary32(exact)
    if integer_bits != expected_integer:
        integer_mismatches.append((bits, expected_integer, integer_bits))
    if float_bits != expected_float:
        float_mismatches.append((bits, expected_float, float_bits))
print(f"Posit-to-int all-pattern checks: 65536; mismatches={len(integer_mismatches)}")
print(f"Posit-to-binary32 all-pattern checks: 65536; mismatches={len(float_mismatches)}")
for mismatch in integer_mismatches[:10] + float_mismatches[:10]:
    print(mismatch)

def round_integer_standard(value):
    if value == -(1 << 31):
        return 0x8000
    if value == 0:
        return 0
    negative = value < 0
    magnitude = abs(value)
    if magnitude < decode(1):
        return 0xffff if negative else 1
    if magnitude >= decode(0x7fff):
        return 0x8001 if negative else 0x7fff
    low, high = 1, 0x7fff
    exact_code = None
    while low + 1 < high:
        middle = (low + high) // 2
        val = decode(middle)
        if val == magnitude:
            exact_code = middle
            break
        if val < magnitude:
            low = middle
        else:
            high = middle
    if exact_code is not None:
        return ((-exact_code) & 0xffff) if negative else exact_code
    if not negative:
        threshold = decode((low << 1) | 1, 17)
        return low if magnitude < threshold or (magnitude == threshold and not (low & 1)) else high
    u, w = (-high) & 0xffff, (-low) & 0xffff
    threshold = decode(((u << 1) | 1) & 0x1ffff, 17)
    return u if Fraction(value) < threshold or (Fraction(value) == threshold and not (u & 1)) else w


integer_values = {0, 1, -1, 2, -2, (1 << 31) - 1, -(1 << 31),
                  (1 << 31) - 2, -(1 << 31) + 1, 1 << 30, -(1 << 30)}
for u in range(1, 0x7fff):
    for threshold in (decode((u << 1) | 1, 17),
                      decode((((-(u + 1)) & 0xffff) << 1 | 1) & 0x1ffff, 17)):
        if threshold.denominator == 1 and abs(threshold.numerator) < (1 << 31):
            integer_values.add(threshold.numerator)
rng = random.Random(20261109)
integer_values.update((rng.getrandbits(32) - (1 << 31)) for _ in range(10000))
integer_cases = sorted(integer_values)
integer_payload = "".join(f"I {value & 0xffffffff:08x}\n" for value in integer_cases)
integer_outputs = subprocess.run([sys.argv[1]], input=integer_payload, text=True,
                                 capture_output=True, check=True).stdout.splitlines()
assert len(integer_outputs) == len(integer_cases)
from_integer_mismatches = []
for value, got in zip(integer_cases, integer_outputs):
    expected = round_integer_standard(value)
    if int(got) != expected:
        from_integer_mismatches.append((value, expected, int(got)))
print(f"int32-to-Posit exact integer cases: {len(integer_cases)}; "
      f"mismatches={len(from_integer_mismatches)}")
for mismatch in from_integer_mismatches[:20]:
    print(mismatch)
raise SystemExit(bool(mismatches or integer_mismatches or float_mismatches or
                     from_integer_mismatches))
