# Schema File

Used to specify the delimiters and variable patterns for compressing and
searching logs. Logs are tokenized using delimiters and each token is classified
as a variable or static text. Variable types are assigned a name and a pattern
defined by the regex rules in the schema file. Some variable names are keywords
and variables with these names are treated specially, Anything not matching a
variable type is considered static text.

## Example Schema File 

```
// Delimiters
delimiters: \t\r\n:,!;%

// Keywords
timestamp:\d{4}\-\d{2}\-\d{2} \d{2}:\d{2}:\d{2}(\.\d{3}){0,1}
timestamp:\[\d{8}\-\d{2}:\d{2}:\d{2}\]
int:\-{0,1}[0-9]+
double:\-{0,1}[0-9]+\.[0-9]+

// Custom variables
hex:[a-fA-F]+
hasNumber:.*\d.*
equals:.*=.*[a-zA-Z0-9].*
```

* `delimiters: \t\r\n:,!;%` indicates that ` `, `\t`, `\r`, `\n`, `:`, `,`, `!`,
  `;`, `%`, and `'` are delimiters. Note, this is not specified in regex and
  every character specified is treated as a delimiter. In a log file, for the
  case of multiple delimiters in a row, e.g., N consecutive spaces, all N spaces
  are stored as static text. Currently, at least one delimiter must be
  specified, and multiple delimiter specifications on separate lines are
  allowed.
* Keywords and custom variables are designated by specifying
  `typeName:regexPattern` where `typeName` consists of alphanumeric characters
  and is followed by a `:`. `regexpattern` can not contain delimiters (except
  `timestamp`) as this is currently incompatible with search. The supported
  regex for `regexpattern` is specified in the section below. The same 
  `typeName` may show up on multiple lines allowing for it to be associated
  with multiple `regexpattern` specifications. Although keywords are defined the
  same as variables, they are treated differently. If a variable in a log
  matches multiple keyword/variable patterns with different names, the pattern
  specified earlier in the schema file is prioritized.
* `timestamp` is a keyword and its pattern may contain delimiters. Timestamps at
  the start of a log file or after a newline in a log file indicates the
  beginning of a new log message. If the log file contains no timestamp at the
  start of the file then a newline is used to indicate the beginning of a new
  log message. Timestamp patterns are not matched midline and are not stored as
  dictionary variables as they may contain delimiters.
* `int` and `double` are keywords. These are encoded specially for compression
  performance.

## Supported Regex
```
REGEX RULE   DEFINITION
ab           match 'a' followed by 'b'
a|b          match a or b
[a-z]        match any character in the brackets (e.g., any lowercase letter)
             - special characters must be escaped in brackets (e.g., [\.\(\\])
[^a-zA-Z]    match any character not in the brackets (e.g., non-alphabet character)
a*           repetition of 'a' 0 or more times
a+           repetition of 'a' 1 or more times
a{N}         repetition of 'a' exactly N times
a{N,M}       repetition of 'a' between N and M times
(abc)        subexpression (concatenates abc)
\d           any digit 0-9
\s           any whitespace ' ', '\r', '\t', '\v', '\f'
.            any non-delimiter character
```

* Regex rules are listed in order of operation

## Known issues
Open an issue on GitHub (https://github.com/y-scope/clp/issues) if you require 
assistance.

* Wildcard search queries for archives compressed with a schema file currently 
  considers `*` to represent 0 or more non-delimiter characters.
* We currently only support ASCII characters. Future support will include UTF-8
  encodings.
