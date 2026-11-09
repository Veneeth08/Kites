import subprocess
import sys

cases = [
    ('A', 0xc000, 0x4000, 0x0000, 'cancellation -1 + 1'),
    ('S', 0x4000, 0x4000, 0x0000, 'cancellation 1 - 1'),
    ('M', 0xc000, 0x4800, 0xb800, 'negative product -1 * 2'),
    ('D', 0xc000, 0x4800, 0xc800, 'negative quotient -1 / 2'),
    ('M', 0x0001, 0x0001, 0x0001, 'positive product under minpos'),
    ('M', 0x0001, 0xffff, 0xffff, 'negative product under minpos'),
    ('D', 0x0001, 0x7fff, 0x0001, 'positive quotient under minpos'),
    ('D', 0xffff, 0x7fff, 0xffff, 'negative quotient under minpos'),
    ('M', 0x7fff, 0x4800, 0x7fff, 'positive overflow saturation'),
    ('M', 0x8001, 0x4800, 0x8001, 'negative overflow saturation'),
    ('M', 0x7cbb, 0x7fbb, 0x7ffd, 'positive regime-transition rounding'),
    ('M', 0x8345, 0x7fbb, 0x8003, 'negative regime-transition rounding'),
    ('M', 0xfff4, 0xffd0, 0x0001, 'positive under-minpos multiplication'),
    ('M', 0x000c, 0xffd0, 0xffff, 'negative under-minpos multiplication'),
    ('D', 0x0001, 0x7fff, 0x0001, 'positive under-minpos division'),
    ('D', 0xffff, 0x7fff, 0xffff, 'negative under-minpos division'),
    ('A', 0x4000, 0x8000, 0x8000, 'NaR rhs addition'),
    ('A', 0x8000, 0x4000, 0x8000, 'NaR lhs addition'),
    ('S', 0x4000, 0x8000, 0x8000, 'NaR rhs subtraction'),
    ('S', 0x8000, 0x4000, 0x8000, 'NaR lhs subtraction'),
    ('M', 0x4000, 0x8000, 0x8000, 'NaR rhs multiplication'),
    ('M', 0x8000, 0x4000, 0x8000, 'NaR lhs multiplication'),
    ('D', 0x4000, 0x8000, 0x8000, 'NaR rhs division'),
    ('D', 0x8000, 0x4000, 0x8000, 'NaR lhs division'),
    ('D', 0x4000, 0x0000, 0x8000, 'positive divide by zero'),
    ('D', 0xc000, 0x0000, 0x8000, 'negative divide by zero'),
    ('D', 0x0000, 0x0000, 0x8000, 'zero divided by zero'),
]
payload = ''.join(f'{op} {a} {b}\n' for op, a, b, _, _ in cases)
proc = subprocess.run([sys.argv[1] if len(sys.argv) > 1 else '/tmp/posit_arith_cli'], input=payload, text=True,
                      capture_output=True, check=True)
actual = [int(line) for line in proc.stdout.splitlines()]
assert len(actual) == len(cases), (len(actual), len(cases))
failures = 0
for case, got in zip(cases, actual):
    op, a, b, expected, label = case
    print(f'{label}: {op}({a:04x},{b:04x}) expected={expected:04x} actual={got:04x} ' +
          ('PASS' if expected == got else 'FAIL'))
    failures += expected != got
raise SystemExit(bool(failures))
