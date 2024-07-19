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

### Custom configuration

The `RegexToWildcardTranslatorConfig` class objects are currently immutable once instantiated. The
constructor takes the following arguments in order:

* `case_insensitive_wildcard`: to be added later along with the character set translation
  implementation.

* `add_prefix_suffix_wildcards`: in the absence of regex anchors, add prefix or suffix wildcards so
  the query becomes a substring query.
  * E.g. `info.*system` gets translated into `*info*system*` which makes the original query a
    substring query.
