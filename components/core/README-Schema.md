# Schema File

The schema file is used to specify the delimiters and variable patterns for 
compressing and searching logs. Logs are tokenized using delimiters and each 
token is classified as a variable or static text. Variable types are assigned a 
name and a pattern defined by the regex rules in the schema file. Some variable 
names are keywords and variables with these names are treated specially.
Anything not matching a variable type is considered to be static text.

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
  `;`, `%`, and `'` are delimiters. Note, this is not a regular expression but a 
  collection of characters; every character specified is treated as a delimiter.
  In a log file, consecutive delimiters, e.g., N consecutive spaces, are stored 
  as static text. Currently, at least one delimiter must be specified, and 
  multiple delimiter specifications on separate lines are allowed.
* Keywords and custom variables are designated by specifying
  `typeName:regexPattern` where `typeName` consists of alphanumeric characters
  and is followed by a `:`. `regexPattern` cannot contain delimiters (except
  `timestamp`) as this is currently incompatible with search. The supported
  regex for `regexPattern` is specified in the section below. The same 
  `typeName` may show up on multiple lines allowing for it to be associated
  with multiple `regexPattern` specifications.
  * Although keywords are defined the same as variables, they are treated 
    differently.
  * If a variable in a log matches multiple keyword/variable patterns with 
    different names, the pattern specified earlier in the schema file is 
    prioritized.
* `timestamp` is a keyword and its pattern may contain delimiters. Timestamps at
  the start of a log file or after a newline in a log file indicate the
  beginning of a new log message. If the log file contains no timestamp at the
  start of the file then a newline is used to indicate the beginning of a new
  log message. Timestamp patterns are not matched midline and are not stored as
  dictionary variables as they may contain delimiters.
* `int` and `double` are keywords. These are encoded specially for compression
  performance.

## Supported Regex
```
REGEX RULE   DEFINITION
ab           Match 'a' followed by 'b'
a|b          Match a OR b
[a-z]        Match any character in the brackets (e.g., any lowercase letter)
             - special characters must be escaped, even in brackets (e.g., [\.\(\\])
[^a-zA-Z]    Match any character NOT in the brackets (e.g., non-alphabet character)
a*           Match 'a' 0 or more times
a+           Match 'a' 1 or more times
a{N}         Match 'a' exactly N times
a{N,M}       Match 'a' between N and M times
(abc)        Subexpression (concatenates abc)
\d           Match any digit 0-9
\s           Match any whitespace character (' ', '\r', '\t', '\v', or '\f')
.            Match any *non-delimiter* character
```

Regex rules are listed in order of operation

## Known issues
Below are current known issues/limitations with schema support. Open an issue on 
[GitHub](https://github.com/y-scope/clp/issues) if you require assistance.

* Wildcard search queries for archives compressed with a schema file currently 
  consider `*` as representing 0 or more non-delimiter characters.
* We currently only support ASCII characters. Future support will include UTF-8
  encodings.
* Timestamps must appear at the start of the message.
* There is currently no way to specify text around a variable that is used for 
  context but is not part of the variable.
