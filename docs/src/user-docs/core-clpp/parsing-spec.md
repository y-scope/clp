# Parsing specifications

A [parsing specification](index.md#terminology) is the set of rules CLP+ uses to parse log messages.
Its file format and regex syntax are defined by log-surgeon and documented in [log-surgeon's parsing
specification reference][spec].

This page shows how to go from the shipped template to a specification that recognizes your own log
formats, and describes the few behaviours specific to CLP+.

## Start from the template

A ready-to-use specification ships with CLP at
`components/package-template/src/etc/parsing-spec.template.txt`, and in the package at
`etc/parsing-spec.template.txt`. It mimics CLP's built-in heuristic-parser and recognizes common
timestamps, numbers, paths, and key-value pairs, making it a reasonable starting point for most
logs.

## Add a rule for your own identifiers

Suppose your logs contain block identifiers such as `blk_1073746484_5660` that you want to query by
their two components. Add a rule naming each component as a sub-rule:

```text
block_id: "blk_(?<num>\d+)_(?<gen_stamp>\d+)"
```

When several rules could match the same text, log-surgeon prefers the longest match; ties are
broken by each rule's optional priority and then by file order. See the [reference][spec] for
details.

Compress a sample and inspect the resulting log shapes to confirm the rule works:

```shell
./clp-s c --experimental --parsing-specification=./my-spec.txt /tmp/archives /tmp/sample.log
./clp-s s --experimental /tmp/archives 'stats.log_shapes'
```

After compression, each leaf rule is a queryable column, named by its fully qualified name and
prefixed by the field the log message came from:

```shell
./clp-s s --experimental /tmp/archives 'message.block_id.num: 1073746484'
```

:::{tip}
A log shape that is entirely static text means nothing was parsed. One that is almost entirely
placeholders usually means a rule is matching too aggressively.
:::

## CLP+-specific behaviour

**Numbers are encoded automatically.** Any leaf rule match whose text is an integer or a decimal
number is stored as a JSON number rather than a string, regardless of the rule's name. This happens
transparently and is what makes numeric comparisons work:

```shell
./clp-s s --experimental /mnt/data/archives 'message.block_id.num > 1073746400'
```

**`header` rules are not used by `clp-s`.** A `header` rule marks where one log message ends and the
next begins, which matters only to the log converter. `clp-s c` ingests JSON, whose log messages are
already separated, so it ignores these rules.

[spec]: https://github.com/y-scope/log-surgeon/blob/log-mechanic/rust/docs/parsing-specification.md
