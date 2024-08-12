# regex_utils library

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

* The translator function returns a `Result<std::string, std::error_code>` type, which can either
contain a value or an error code.

To use the translator:

```cpp
#include <regex_utils/regex_translation_utils.hpp>

using clp::regex_utils::regex_to_wildcard;

// Other code

auto result{regex_to_wildcard(wildcard_str)};
if (result.has_error()) {
    auto err_code{result.error()};
    // Handle error
} else {
    auto regex_str{result.value()};
    // Do things with the translated string
}
```

* To add custom configuration to the translator:

```cpp
#include <regex_utils/RegexToWildcardTranslatorConfig.hpp>

RegexToWildcardTranslatorConfig config{true, false, /*...other booleans*/};
auto result{regex_to_wildcard(wildcard_str, config)};

// Same as above
```

For a detailed description on the options order and usage, see the
[Custom Configuration](#custom-configuration) section.

### Functionalities

* Wildcards
  * Turn `.` into `?`
  * Turn `.*` into `*`
  * Turn `.+` into `?*`
  * E.g. `abc.*def.ghi.+` will get translated to `abc*def?ghi?*`
* Metacharacter escape sequences
  * An escaped regex metacharacter is treated as a literal and appended to the wildcard output.
    * The list of characters that require escaping to have their special meanings suppressed is
      `[\/^$.|?*+(){}`.
    * Superfluous escape characters are ignored for the following characters: `],<>-_=!`.
    * E.g. `a\[\+b\-\_c-_d` will get translated to `a[+b-_c-_d`
    * Note: generally, any non-alphanumeric character can be escaped to use it as a literal. The
      list this utils library supports is non-exhaustive and can be expanded when necessary.
  * For metacharacters shared by both syntaxes, keep the escape backslashes.
    * The list of characters that fall into this category is `*?\`. All wildcard metacharacters are
      also regex metacharacters.
    * E.g. `a\*b\?c\\d` will get translated to `a\*b\?c\\d` (no change)
  * Escape sequences with alphanumeric characters are disallowed.
    * E.g. Special utility escape sequences `\Q`, `\E`, `\A` etc. and back references `\1` `\2` etc.
      cannot be translated.
* Character set
  * Reduces a character set into a single character if possible.
    * A trivial character set containing a single character or a single escaped metacharacter.
      * E.g. `[a]` into `a`, `[\^]` into `^`
    * If the `case_insensitive_wildcard` config is turned on, the translator can also reduce the
      case-insensitive style character set patterns into a single lowercase character:
      * E.g. `[aA]` into `a`, `[Bb]` into `b`, `[xX][Yy][zZ]` into `xyz`

### Custom configuration

The `RegexToWildcardTranslatorConfig` class objects are currently immutable once instantiated. By
default, all of the options are set to `false`.

The constructor takes the following option arguments in order:

* `case_insensitive_wildcard`: see **Character set** bullet point in the
  [Functionalities](#functionalities) section.

* `add_prefix_suffix_wildcards`: in the absence of regex anchors, add prefix or suffix wildcards so
  the query becomes a substring query.
  * E.g. `info.*system` gets translated into `*info*system*` which makes the original query a
    substring query.
