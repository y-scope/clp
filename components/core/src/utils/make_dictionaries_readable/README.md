This program converts an archive's dictionaries into human-readable form.
For a dictionary, `make-dictionaries-readable` prints one entry per line.

For log type dictionary entries, this requires making some characters printable:

* Newlines are replaced with `\n`
* Dictionary variable placeholders are replaced with `\d`
* Non-dictionary integer variable placeholders are replaced with `\i`
* Non-dictionary float variable placeholders are replaced with `\f`
