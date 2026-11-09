# Posit16 ES=2 Independent Reference Checks

These utilities compare Kites' Posit16 implementation with an independent
Python oracle based on the *Posit Standard (2022)*. The Python oracle uses
`fractions.Fraction`, parses the Standard's raw bit fields, evaluates its exact
value equation, and applies Section 4.1 rounding. It does not import or call
Kites `decode()`, `encode()`, or arithmetic functions.

The C++ files in this directory are adapters only. They expose Kites outputs to
the Python oracle; they do not define expected posit values. The C++ adapters
are `posit_decode_dump.cpp`, `posit_encode_cli.cpp`, and `posit_arith_cli.cpp`.

## Build the adapters

Run from the repository root. These commands write binaries and dumps under
`/tmp/posit-reference-build`, not into production source directories:

```sh
mkdir -p /tmp/posit-reference-build
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp test/posit_reference/posit_decode_dump.cpp -o /tmp/posit-reference-build/posit_decode_dump
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp test/posit_reference/posit_encode_cli.cpp -o /tmp/posit-reference-build/posit_encode_cli
g++ -std=c++20 -O2 -Wall -Wextra -Werror -Isrc src/processor/posit.cpp test/posit_reference/posit_arith_cli.cpp -o /tmp/posit-reference-build/posit_arith_cli
```

## Run the independent comparisons

```sh
/tmp/posit-reference-build/posit_decode_dump > /tmp/posit-reference-build/decode.txt
python3 test/posit_reference/posit_standard_decode_check.py /tmp/posit-reference-build/decode.txt
python3 test/posit_reference/posit_standard_encode_roundtrip.py /tmp/posit-reference-build/posit_encode_cli
python3 test/posit_reference/posit_standard_round_check.py /tmp/posit-reference-build/posit_encode_cli
python3 test/posit_reference/posit_standard_arithmetic_check.py /tmp/posit-reference-build/posit_arith_cli
python3 test/posit_reference/posit_targeted_cases.py /tmp/posit-reference-build/posit_arith_cli
```

The rounding checker covers all 32,766 positive and all 32,766 negative
Section 4.1 `U1` thresholds, plus signed values below minPos. For negative
thresholds it constructs the lower numeric raw code `U`, appends the `1` bit
to form the 17-bit code, and applies the lower `U` LSB rule directly. The
arithmetic oracle uses the same exact Standard rule and deterministic pair
sampling with seed `20261109`.

## Strict compile and UBSan

```sh
g++ -std=c++20 -Wall -Wextra -Werror -Wconversion -Wsign-conversion -pedantic -Isrc -fsyntax-only src/processor/posit.cpp
g++ -std=c++20 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -Isrc src/processor/posit.cpp test/posit_reference/posit_decode_dump.cpp -o /tmp/posit-reference-build/posit_decode_dump_ubsan
/tmp/posit-reference-build/posit_decode_dump_ubsan > /tmp/posit-reference-build/decode_ubsan.txt
python3 test/posit_reference/posit_standard_decode_check.py /tmp/posit-reference-build/decode_ubsan.txt
g++ -std=c++20 -O1 -g -fsanitize=undefined -fno-sanitize-recover=all -Wall -Wextra -Werror -Isrc src/processor/posit.cpp test/posit_reference/posit_arith_cli.cpp -o /tmp/posit-reference-build/posit_arith_cli_ubsan
UBSAN_OPTIONS=halt_on_error=1 python3 test/posit_reference/posit_targeted_cases.py /tmp/posit-reference-build/posit_arith_cli_ubsan
```

## Reference scope and history

- Normative source: [Posit Standard (2022)](https://posithub.org/docs/posit_standard-2.pdf), especially §§3.3 and 4.1.
- Stillwater Universal was inspected at its `main` source URL during the audit,
  but no exact commit SHA or executable comparison was available. It is not
  used to produce these oracle outputs.
- `posit_core_check_legacy.cpp` preserves the earlier hand-written harness for
  historical context. It contains pre-audit underflow expectations and must
  not be treated as a current conformance test or run as the reference oracle.
- The original Python scripts and adapters were found under `/tmp` and copied
  here before changes. The Standard-based Python logic remains independent;
  the round and arithmetic scripts were extended to apply the signed `U1`
  rule directly and accept adapter paths as arguments.
