# Schema File

Used to specify the delimiters and variable patterns for compressing and searching logs. Logs are tokenized using delimiters and classified as a variable type or static text. Variable types are defined by the regex rules in the schema file. Anything not matching a variable type is considered static text.

## Example Schema File 

```
// Delimiters
delimiters: \t\r\n:,=!;%

// Keywords
timestamp:[0-9]{4}\-[0-9]{2}\-[0-9]{2} [0-9]{2}:[0-9]{2}:[0-9]{2}(\.[0-9]{3}){0,1}
timestamp:\[\d{8}\-\d{2}:\d{2}:\d{2}\]
int:\-{0,1}[0-9]+
double:\-{0,1}[0-9]+\.[0-9]+

// Custom variables
hex:[a-fA-F]+
hasNumber:.*\d.*
equals:.*=.*[a-zA-Z0-9].*
```
* `delimiters: \t\r\n:,=!;%` indicates that ` `, `\t`, `\r`, `\n`, `:`, `=`, `!`, `;`, `%`, and `'` are delimiters. Note this is not specified in regex and every character specified is treated as a delimiter. In a log file, for the case of multiple delimiters in a row, i.e., N consecutive spaces,all N spaces are stored as static text and the remaining behaviour is the same as a single delimiter. Currently, at least 1 delimiter must be specified, and multiple delimiter specifications on separate lines is allowed. 
* Keywords and custom variables are designated by specifying `typeName:regexPattern` where `typeName` consists of alphanumeric characters and is followed by a `:`. `regexpattern` can not contain delimiters (except `timestamp`) as this is currently incompatible with search. The supported regex for `regexpattern` is specified in the section below. The same `typeName` may show up on multiple lines allowing for it to be associated with multiple `regexpattern` specifications. Although keywords are defined the same as variables, they are treated differently. If a variable in a log matches multiple keyword/variable patterns, the pattern specified earlier in the schema file is prioritized.
* `timestamp` is a keyword and its pattern may contain delimiters. A timestamps at the start of a log file or after a newline in a log file indicates the beginning of a new log message. If the log file contains no timestamp at the start of the file then a newline is used to indicate the beginning of a new log message. Timestamp patterns are not matched midline and are not stored as dictionary variables as they may contain delimiters. 
* `int` and `double` are keywords. These are encoded specially for compression performance.

## Supported Regex
```
REGEX RULE      DEFINITION
ab              concatenates 'a' and 'b'
a|b             union of a and b
[a-zA-z]        bracket expression (e.g., union of lower case and uper case letters)
                - special characters must still be cancelled inside brackets (e.g., [\.\(\)\\])
[^a-zA-z]       negating (e.g., any non-alphabet character)
(abc)           subexpression (concatenates abc)
(abc)*          repitition of 'abc' 0 or more times
(abc)+          repitition of 'abc' 1 or more times
(abc){N}        repitition of 'abc' exactly N times
(abc){N,M}      repitition of 'abc' between N and M times
\d              any digit 0-9
\s              any whitespace ' ', '\r', '\t', '\v', '\f'
.               any non-delimiter character
```

* Regex rules are listed in order of operation

## Known issues
Open an issue on GitHub (https://github.com/y-scope/clp/issues) if you require assistance.
* Search using a schema file considers `*` to represent 0 or more non-delimiter characters.
* Only currently supports ASCII characters, future support will include utf-8 encodings.
