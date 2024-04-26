# make-dictionaries-readable

This program converts an archive's dictionaries into a human-readable form.

Usage:

:::{code-block} shell
./make-dictionaries-readable <archive-path> <output-dir>
:::

* `archive-path` is a path to a specific archive (inside `archives-dir`)


## Format

For a dictionary, `make-dictionaries-readable` prints one entry per line.

For log type dictionary entries, this requires making some characters printable:

* Newlines are replaced with `\n`
* Dictionary variable placeholders are replaced with `\d`
* Non-dictionary integer variable placeholders are replaced with `\i`
* Non-dictionary float variable placeholders are replaced with `\f`
