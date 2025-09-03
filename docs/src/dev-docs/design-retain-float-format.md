# Retain Float Format

:::{warning}
🚧 This section is still under construction.
:::

## Float Format Encoding

Similar to `ClpString`, a `FormattedFloat` node stores the original formatting of a floating-point
value alongside its numeric value.  

Each `FormattedFloat` node contains:

- The double value in IEEE 754 64-bit format.
- A 2-byte *format* field encoding the necessary output formatting information so that, upon
  decompression, the value can be reproduced to match the original text as closely as possible
  (see limitations below). Note that the remaining bits of the 2‑byte field are currently reserved.
  Encoders must write them as 0, and decoders must ignore them (treat as “don’t care”) for forward
  compatibility.

```text
+-------------------------------------+------------------------+--------------------------+------------------------------------------------------+
| Scientific Notation Marker (2 bits) | Exponent Sign (2 bits) | Exponent Digits (2 bits) | Digits from First Non-Zero to End of Number (4 bits) |
+-------------------------------------+------------------------+--------------------------+------------------------------------------------------+
```

### Scientific Notation Marker

Indicates whether the number is in scientific notation, and if so, whether the exponent is denoted
by `E` or `e`.

- `00`: Not scientific
- `01`: Scientific notation using `e`
- `11`: Scientific notation using `E`

`10` is unused so that the lowest bit can act as a simple “scientific” flag, making condition
checks cleaner.

### Exponent Sign

Records whether the exponent has a sign:

- `00`: No sign
- `01`: `+`
- `10`: `-`

`11` is reserved (encoders must write 0; decoders must ignore).

For example, exponents of `0` may appear as `0`, `+0`, or `-0`, and these two bits can record the
format correctly.

### Exponent Digits

Since the maximum decimal exponent for a double is `308` (three digits), two bits are enough to
represent the digit count (range 1–4 digits to allow preserving leading zeros).

The stored value is **actual digits − 1**, since there is always at least one digit
(e.g., `00` → 1 digit). The two-bit mapping is:

- `00` → 1 digit
- `01` → 2 digits
- `10` → 3 digits
- `11` → 4 digits

Note that the stored value is not applied **strictly**.

- If the decompressed double’s scientific exponent has fewer digits than the stored value, leading
  zeros are inserted until the digit counts match.
- If it has more digits than the stored value, leading zeros are trimmed where possible until they
  match.

However, the latter case is not guaranteed. For example, a value like `123456789123456.7E+1` has a
one-digit exponent, but as noted in the CLP-S's limitations, decompression converts it to
`1.234567891234567E+15`. Here, trimming either `1` or `5` would alter the value, so the exponent
remains two digits rather than one. In such cases, **correctness takes precedence over preserving
the exact format**; this rule supersedes all formatting metadata to ensure numeric fidelity.

### Digits from First Non-Zero to End of Number

This counts the digits from the first non-zero digit up to the last digit of the integer or
fractional part (excluding the exponent). Examples:

- `123456789.1234567000` → **19** (from first `1` to last `0`)
- `1.234567890E16` → **10** (from first `1` to `0` before exponent)
- `0.000000123000` → **6** (from first `1` to last `0`)
- `0.00` → **3** (counts all zeros for zero value)

According to IEEE 754 (64-bit), more than 16 significant digits (for non-zero values) may lead to
precision loss. Thus, the maximum stored value is 16. This limit also applies to zero; for example,
if a zero value contains more than 16 digits in total, it will be trimmed to 16 digits.

Per the [JSON grammar][json_grammar], the integer part cannot be empty, so the minimum is **1** digit.  
We store this as **actual digits − 1**, allowing representation with 4 bits (`0000` → 1 digit).

[json_grammar]: https://www.crockford.com/mckeeman.html
