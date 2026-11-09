from fractions import Fraction

def p2(e): return Fraction(1 << e, 1) if e >= 0 else Fraction(1, 1 << -e)

def dec(raw):
    if raw == 0: return Fraction(0)
    if raw == 0x8000: return None
    s = (raw >> 15) & 1
    cur = 14
    rb = (raw >> cur) & 1
    run = 0
    while cur >= 0 and ((raw >> cur) & 1) == rb:
        run += 1; cur -= 1
    r = run - 1 if rb else -run
    if cur >= 0: cur -= 1
    e = 0
    for _ in range(2):
        e = (e << 1) | (((raw >> cur) & 1) if cur >= 0 else 0)
        if cur >= 0: cur -= 1
    m = max(cur + 1, 0)
    f = raw & ((1 << m) - 1) if m else 0
    return (Fraction(1 - 3*s, 1) + Fraction(f, 1 << m)) * p2((1 - 2*s) * (4*r + e + s))

for a,b in [(0x7cbb,0x7fbb),(0xfff4,0xffd0)]:
    va,vb = dec(a),dec(b)
    print(f'{a:04x} = {va} ({float(va):.17g})')
    print(f'{b:04x} = {vb} ({float(vb):.17g})')
    print(f'exact product = {va*vb} ({float(va*vb):.17g})')
    print(f'expected standard result bits = {0x7ffd if a == 0x7cbb else 0x0001:04x}')
