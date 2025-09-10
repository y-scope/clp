# Retaining Floating-Point Format Information

To losslessly retain the string representation of floating point numbers we use two encoding
strategies:
* `FormattedFloat`: similar to `DateString`, we store formatting information about a floating point
number alongside its IEEE-754 binary64 representation.
* `DictionaryFloat`: we store the full string representation of the floating point number in the
variable dictionary, and encode numbers as their corresponding variable dictionary IDs.

Generally we prefer storing floating point numbers as `FormattedFloat` over `DictionaryFloat`
because:
* We can directly compare against the stored IEEE-754 binary64 float at query time instead of having
to first parse the string representation of a floating point number.
* We avoid bloating the variable dictionary with non-repetitive floating point strings.

Unfortunately, even though `FormattedFloat` is designed to represent most common encodings of
IEEE-754 binary64 floats, we can not guarantee that our input follows a common format or was
converted from a binary64 floating point number. As a result, at parsing time, we check if a given
floating point number is representable as a `FormattedFloat`, and if it isn't, we encode it as a
`DictionaryFloat`.

## High-Level `FormattedFloat` Specification  

Each `FormattedFloat` node contains:

- The double value in IEEE-754 binary64 format.
- A 2-byte *format* field encoding the necessary output formatting information so that, upon
  decompression, the value can be reproduced to match the original text as closely as possible.
  Note that the remaining bits of the 2‑byte field are currently reserved.
  Encoders must write them as 0, and decoders must ignore them (treat as “don’t care”) for forward
  compatibility.

```text
+-------------------------------------+------------------------+--------------------------+------------------------------------------------------+
| Scientific Notation Marker (2 bits) | Exponent Sign (2 bits) | Exponent Digits (2 bits) | Digits from First Non-Zero to End of Number (5 bits) |
+-------------------------------------+------------------------+--------------------------+------------------------------------------------------+
```

To clarify the floating point formats that `FormattedFloat` can represent, we describe them in text
here:
* For non-scientific numbers we accept:
  * Any number that has at most 16 digits after the first non-zero digit
  * Or at most 1 zero before the decimal and 16 zeroes after the decimal, if the number is a zero
* For scientific numbers we accept:
  * Single digit numbers with no decimal, followed by an exponent
  * Or numbers with **1** digit preceding the decimal and up to 16 digits following the
    decimal, followed by an exponent
  * Where zero can not be the digit before the decimal, unless the number is a zero
  * And where the exponent is specified by `e` or `E` optionally followed by `+` or `-`
  * With at most **4** exponent digits, which must be left-padded with `0`

With the added restrictions that:
* The floating point number follows the JSON grammar for floating point numbers.
* There exists an IEEE-754 binary64 number for which the string is the closest decimal
  representation at the given precision.

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

`11` is unused by the current implementation.

For example, exponents of `0` may appear as `0`, `+0`, or `-0`, and these two bits can record the
format correctly.

### Exponent Digits

Since the maximum and minimal decimal exponents for a double, `308` and `-324 respectively`, are
both three digits, two bits are enough to represent the digit count.

The stored value is **actual digits − 1**, since there is always at least one digit
(e.g., `00` → 1 digit). The two-bit mapping is:

- `00` → 1 digit
- `01` → 2 digits
- `10` → 3 digits
- `11` → 4 digits

### Digits from First Non-Zero to End of Number

This counts the digits from the first non-zero digit up to the last digit of the integer or
fractional part (excluding the exponent). Examples:

- `123456789.1234567000` → **19** (from first `1` to last `0`)
- `1.234567890E16` → **10** (from first `1` to `0` before exponent)
- `0.000000123000` → **6** (from first `1` to last `0`)
- `0.00` → **3** (counts all zeros for zero value)

Per the [JSON grammar][json_grammar], the integer part of a floating point number can not be empty,
so the minimum number of digits is **1**. To take advantage of this fact, we store this field as
**actual number of non-zero digits to end of number - 1**.

As well, according to IEEE-754, only 17 decimal significant digits are needed to represent all
binary64 floating point numbers without precision loss. As a result, we currently choose to allow
a maximum value of **17 digits** in this field. Unfortunately, even with our encoding scheme, this
requires 5 bits to store the maximum encoded value of 16.

We could support representing binary64 numbers with up to 32 significant digits, and we may choose
to do so in the future, but this is explicitly not supported in the current version of the format.
The rationale for not doing so now is that as the number of digits increases beyond 17, the
likelihood that the number corresponds to a valid IEEE-754 binary64 float decreases.

[json_grammar]: https://www.crockford.com/mckeeman.html
