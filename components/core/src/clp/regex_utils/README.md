# Regex_utils

This library contains useful utilities to handle all regex related tasks.

## Regex to Wildcard Translator

### Goal

Performs a best-effort translation to turn a regex string to an equivalent wildcard string.

CLP currently only recognizes three meta-characters in the wildcard syntax:

* `?` Matches any single character
* `*` Matches zero or more characters
* `\` Suppresses the special meaning of meta characters (including itself)

If the regex query can actually be expressed as a wildcard query only deploying the three
metacharacters above, CLP should use the wildcard version.

### Includes

* To use the translator:

```shell
#include <regex_utils/regex_translation_utils.hpp>
```

* To add custom configuration to the translator:

```shell
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>
```

### Functionalities

* Wildcards
  - Turn `.` into `?`
  - Turn `.*` into `*`
  - Turn `.+` into `?*`

### Custom configuration

* `add_prefix_suffix_wildcards`: in the absence of regex anchors, add prefix or suffix wildcards so
the query becomes a substring query.
  - E.g. `info.*system` gets translated into `*info*system*` which makes the original query a
  substring query.
