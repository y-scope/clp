# Lossless JSON float storage

Since our goal is to losslessly retain floating-point numbers that come from JSON input, it is worth
taking a look at what kinds of floating-point numbers can appear in JSON.

The [JSON specification][json_spec] treats fields that match the following grammar as number values:

```text
number        = [ minus ] int [ frac ] [ exp ]

int           = zero | ( digit1-9 digit* )
frac          = decimal-point digit+
exp           = e [ minus | plus ] digit+

digit         = zero | digit1-9
digit1-9      = '1'-'9'
zero          = '0'

e             = 'e' | 'E'
decimal-point = '.'
minus         = '-'
plus          = '+'
```

For our purposes, floating-point numbers are numbers which match this grammar and have either a
fraction, an exponent, or both.

Note that this restricts which floating‑point numbers are allowed in a few ways:

- NaN and +/- Infinity are not allowed
- The exponent must contain at least one digit
- The fractional part of a number must contain at least one digit
- The integer part of a number can only start with '0' if the entire integer part is '0'
- The integer part of a number must contain at least one digit
- Positive numbers cannot begin with an explicit '+'

There are two important points to note about this grammar:
1. It doesn't place any restrictions on how a given floating-point number _should_ be written
2. It doesn't dictate whether or not floating-point numbers have to correspond to values from a
   standard such as IEEE-754 binary64.

Since point 1. is less abstract than point 2., we'll explain it first. Say we're trying to represent
the number `16` as a floating-point number using **3** digits of precision. We might write it as:

- `16.0`; or
- `16.0e0`; or
- `1.60e1`

Note that for scientific-notation representations of a number, we can shift the decimal and change
the exponent arbitrarily to represent the same number in infinite possible ways. For example:

- `0.160e2`; and
- `0.00000000160e10`

are both valid representations of `16` using 3 significant digits.

Likewise, we can come up with infinite representations of `16` (at different precisions) by
choosing to represent arbitrarily many significant digits.

Point 2. is a bit more abstract, but it is important for understanding our approach to losslessly
storing floating-point numbers.

It's most easily demonstrated with an example. Of the numbers:

- `1.2345678901234567`
- `1.2345678901234568`
- `1.2345678901234570`

we know that only the first and third number correspond to IEEE-754 binary64 floating-point numbers.
The reason we can tell is that the [IEEE-754 specification][ieee754] requires that when converting a
floating-point number to a decimal string at a given precision, the decimal string must correspond
to the nearest decimal representation. Likewise, when converting a decimal string to a
floating-point number, the standard requires that the number be converted to the nearest
floating-point representation. If you use any standards-compliant implementation to turn
`1.2345678901234568` into a binary64 floating-point number, and back to a decimal string, you will
find that it has been rounded to `1.2345678901234567`.

Overall the implications here are that:
- For any given number there are many possible representations (infinitely many in fact); and
- Not all floating-point numbers that are valid JSON correspond to values from a standard like
  IEEE-754

In practice though, we know that most of the time we should be dealing with very standard
machine-generated data. This means that most inputs _do_ correspond to IEEE-754 binary64
floating-point numbers, and that of the infinitely many ways of representing a number only a few
will be common.

Our approach, then, is to store most floating-point numbers as an IEEE-754 binary64 floating-point
number alongside some formatting information, falling back to storing the number as a string when
that doesn't work.

## Retaining floating-point format information

To losslessly retain the string representation of floating-point numbers we use two encoding
strategies:

- `FormattedFloat`: similar to `DateString`, we store formatting information about a floating-point
number alongside its IEEE-754 binary64 representation.
- `DictionaryFloat`: we store the full string representation of the floating-point number in the
variable dictionary, and encode numbers as their corresponding variable dictionary IDs.

Generally we prefer storing floating-point numbers as `FormattedFloat` over `DictionaryFloat`
because:

- `FormattedFloat` allows us to directly compare against the stored IEEE-754 binary64 float at query
  time instead of having to first parse the string representation of a floating-point number.
- Fewer `DictionaryFloat` encodings means that we can avoid bloating the variable dictionary with
  non-repetitive floating-point strings.

Unfortunately, even though `FormattedFloat` is designed to represent most common encodings of
IEEE-754 binary64 floats, we cannot guarantee that our input follows a common format or was
converted from a binary64 floating-point number. As a result, at parsing time, we check if a given
floating-point number is representable as a `FormattedFloat`, and if it isn't, we encode it as a
`DictionaryFloat`.

## High-level `FormattedFloat` specification

Each `FormattedFloat` node contains:

- The double value in IEEE-754 binary64 format.
- A 2-byte little-endian _format_ field encoding the necessary output formatting information so
  that, upon decompression, the value can be decompressed exactly to the original text.

Note that the unused lowest 5 bits of the 2‑byte field are currently reserved. Encoders must write
them as 0, and decoders must ignore them (treat as “don’t care”) for forward compatibility.

From MSB to LSB, the 2-byte format field contains the following sections:

- [Scientific notation marker](#scientific-notation-marker) (2 bits)
- [Exponent sign](#exponent-sign) (2 bits)
- [Exponent digits](#exponent-digits) (2 bits)
- [Digits from first non-zero to end of number](#digits-from-first-non-zero-to-end-of-number) (5 bits)
- Reserved for future use (5 bits)

The floating-point formats that `FormattedFloat` can represent are described below:

- Numbers not written in scientific notation are accepted if **either** of the following are true:
  1. Any non-zero number with at most **17** digits starting at the first non-zero digit
  2. A zero written with at most **one** zero before the decimal point and at most **16** zeros
     after the decimal point
- Numbers written in scientific notation are accepted if all **four** of the following are true:
  1. The significand is either
      * a **single** digit with no decimal point, followed by an exponent
      * **one** digit before the decimal point, and up to **16** digits after the decimal point,
        followed by an exponent
  2. The digit before the decimal point cannot be zero unless every digit of the significand is zero
  3. The exponent uses `e` or `E`, optionally followed by `+` or `-`
  4. The exponent has at most **4** digits (left-padding with `0` is allowed)

With the added restrictions that:

- The floating-point number follows the JSON grammar for floating-point numbers.
- There exists an IEEE-754 binary64 number for which the string is the closest decimal
  representation at the given precision.

These restrictions really correspond to "canonical" representations of floating-point numbers with
up to 17 digits of precision. This means that our formatting scheme can always represent numbers
produced by format specifiers such as `%f`, `%e`, and `%g`, so long as they don't use too many
digits of precision, and the underlying number isn't NaN, or +/- Infinity.

### Scientific notation marker

Indicates whether the number is in scientific notation, and if so, whether the exponent is denoted
by `E` or `e`.

- `00`: Not scientific
- `01`: Scientific notation using `e`
- `11`: Scientific notation using `E`

`10` is unused so that the lowest bit can act as a simple “scientific” flag, making condition
checks cleaner.

### Exponent sign

Records whether the exponent has a sign:

- `00`: No sign
- `01`: `+`
- `10`: `-`

`11` is unused by the current implementation.

For example, exponents of `0` may appear as `0`, `+0`, or `-0`, and these two bits can record the
format correctly.

### Exponent digits

Since the maximum and minimum decimal exponents for a double (`308` and `-324` respectively) are
both three digits, two bits are enough to represent the digit count. We allow up to 4 digits to
support exponents left-padded with `0`.

The stored value is **one less than the number of actual digits**, since there is always at least
one digit (e.g., `00` → 1 digit). The two-bit mapping is:

- `00` → 1 digit
- `01` → 2 digits
- `10` → 3 digits
- `11` → 4 digits

### Digits from first non-zero to end of number

This counts the digits from the first non-zero digit up to the last digit of the integer or
fractional part (excluding the exponent). Examples:

- `123456789.1234567000` → **19** (from first `1` to last `0`)
- `1.234567890E16` → **10** (from first `1` to `0` before exponent)
- `0.000000123000` → **6** (from first `1` to last `0`)
- `0.00` → **3** (counts all zeros for zero value)

Per the [JSON specification][json_spec], the integer part of a floating-point number cannot be
empty, so the minimum number of digits is **1**. To take advantage of this fact, we store this field
as **one less than the actual number of digits from the first non-zero digit to the end of the
number**; for the numeric value zero we store **one less than the actual number of digits**.

As well, according to IEEE-754, only 17 decimal significant digits are needed to represent all
binary64 floating-point numbers without precision loss. As a result, we currently allow a maximum of
**17 digits**. Because the stored value is **digits - 1** the maximum encoded value is 16, which
requires 5 bits.

We could support representing binary64 numbers with up to 32 significant digits, and we may choose
to do so in the future, but this is explicitly not supported in the current version of the format.
The rationale for not doing so now is that the likelihood that a number corresponds to a valid
IEEE-754 binary64 float decreases as the number of digits increases beyond 17.

[json_spec]: https://datatracker.ietf.org/doc/html/rfc8259
[ieee754]: https://ieeexplore.ieee.org/document/4610935/
