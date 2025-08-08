# Retain Float Format

:::{warning}
🚧 This section is still under construction.
:::

## Float Format Encoding

Similar to `ClpString`, the format will be stored as extra information on top of the value for a new node type `FormattedFloat`.
Each `FormattedFloat` node will have a double value stored in IEEE 754 (64 bits) format, and a 2-bytes *format* to store necessary
format information that used for output when decompression the double value in the exactly same format before compression.

```
+-------------------------------------+------------------------+--------------------------+---------------------------------------------+
| Scientific Notation Marker (2 bits) | Exponent Sign (2 bits) | Exponent Digits (2 bits) | Digits from First Non-Zero to End of Number |
+-------------------------------------+------------------------+--------------------------+---------------------------------------------+
```

### Scientific Notation Marker

This part is to mark if the number is scientfic or not. If so, is it using `E` or `e` to denote the exponent. The possible values for these
two bits:

* `00`: Not scientific.
* `01`: It is using `e`.
* `11`: It is using `E`.

`10` is skipped because the right bit is used for checking if it is use scientific if it is set. This can make the condition statement cleaner.

### Exponent Sign

This part is to record if the exponent has any sign. For the positive exponent it could have an optional `+`. If exponent is 0, it could be `+0` or `-0` or `0`.
So we simply record if there is no sign, or the sign is `+` or `-`, these three possibilities is determined by 2 bits:

* `00`: No sign.
* `01` `+`.
* `10` `-`.

### Exponent Digits

The maximum exponent in double precision is 385, which has three digits. So we need only 2 bits to store the exponent digits. Since there is at least one digit, so
we use the binary value of these two bits plus one is the actual number of exponent digits (e.g., `00` means the exponent has one digit).

### Digits from First Non-Zero to End of Number

For example:

* for `123456789.1234567000`, it is 19, from the start `1` to end last `0`.
* for `1.234567890E16`, it is 10, from the start `1` to the `0` before the exponent. It does not counts exponent part.
* for `0.000000123000`, it is 6, from the `1` to the last `0`.
* for `0.00`, it is 3. When the number is 0, it counts how many zeros.

According to the IEEE 754 (64 bits), when this digits excceed 16 (if the number is not zero), then there might be precision loss. So we allow the maximum digits from first non-zero to end of number as 16.
So 
