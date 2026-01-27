# Tagged Deterministic Finite Automata

This document explains the inner workings of the deterministic finite automaton (DFA) implemented in
LogSurgeon and used for compression and search in CLP.

## Contents

- [Schema](#1-schema)
- [NFA](#2-nfa)
- [DFA](#3-dfa)
- [Tagged DFA](#4-tagged-dfa)
- [Compression](#5-compression-example)
- [Search](#6-search)

## 1. Schema

A schema is an ordered collection of user-defined rules that describe how log messages should be
interpreted by identifying variables and static-text:

- **Variables** are text in the log that contain information pertinent to the user.
- **Static-text** is the remaining, non-variable, text in the log.

The schema consists of two primary components:

- **Delimiters**, which define variable boundaries
- **Regex rules**, which define higher-level patterns matched over the input

### Delimiters

Delimiters are characters that define the boundaries of variables and static-text. While variables
can contain delimiters, all variables must be surrounded by delimiters (or the end/start of the
log).

Delimiters are used to ensure that variables are matched in the valid context. They do not tokenize
or split the log.

### Regex Rules

Regex rules define patterns that match sequences of characters. Each regex rule contains a variable
name and a regex pattern. Examples include:

```text
| Variable Name   | Regex Pattern              | Input      |  Match |
|-----------------|----------------------------|------------|--------|
| float           | \-?\d+\.\d+                | 12.34      | 12.34  |
| user_id         | user_id=(?<user_id>\d+)    | user_id=55 | 55     |
```

Each regex rule is used to construct an NFA, which is eventually considered during DFA construction.

### Priority and Ordering

When multiple regex rules match the same input, the ordering defined in the schema is used to
determine priority.

### Example Schema

```regex
delimiters: \n\r\t

int:-?\d+
float:-?\d+\.\d+
tagged_user_id:user_id=(?<user_id>\d+)
```

## 2. NFA

A nondeterministic finite automaton (NFA) is a state machine where each state can have multiple
outgoing transitions from the same input symbol, including epsilon transitions (transitions that
consume no input).

In LogSurgeon, NFAs are built from the regex rules. They form an intermediate representation between
user-defined regex rule and DFAs.

### Construction

Each regex rule is compiled into an individual NFA:
- Literal character produces linear sequences of states.
- Characters classes, quantifiers, and optional segments produce branches.

The final state of the sequence is marked as an accepting state, indicating the regex has been
matched.

Multiple NFAs can be combined into a single NFA representing the entire schema. This allows matching
any regex rule from a single starting point.

### States and Transitions

NFA states are connected by labeled transitions:
- **Literal transitions** consume a specific character.
- **Epsilon transitions** do not consume any character and are always taken.

At this stage, a single input can lead to multiple possible next states. This nondeterminism is the
reason NFAs are inefficient for traversal. For runtime performance, an NFA must be converted to
a DFA.

## 3. DFA

A deterministic finite automaton (DFA) consists of a finite set of states connected by labeled
transitions. At any point during traversal, the automaton is in exactly one state, and for a given
input symbol there is at most one outgoing transition.

In LogSurgeon, DFAs are used as the execution model for lexing logs.

### States and Transitions

Each DFA state represents a set of possible NFA states. Transitions are labeled by literal bytes.


For any state `S` and input symbol `c`, there is at most one transition:

```text
S--c-->S'
```

This property guarantees deterministic execution.

### Deterministic Traversal

Traversal proceeds left-to-right over the input:

1. Start in the initial DFA state.
2. For each input symbol:
   - follow the unique matching transition,
   - or fail if no transition exists.
3. If no failure occurs above, end in a single final state.

No backtracking or ambiguity resolution occurs during traversal. This makes DFA execution:
- Linear runtime in the input size.
- Predictable in performance.
- Suitable for high-throughput parsing and search.

### Acceptance and Match Semantics
A DFA state is marked as **accepting** if any of its corresponding NFA states are accepting,
meaning the input so far matches an underlying regex pattern. As a DFA state can map to multiple
accepting NFA states, an accepting DFA state can indicate a match for multiple regex patterns.

A match succeeds if the given input has a traversal that does not fail and ends in an accepting
state. At this level the DFA only indicates a list of matches, ordered by variable priority in the
schema. This type of non-tagged DFA does not produce any semantic information beyond the matching
variable types; capture semantics and match metadata are introduced by the tagged DFA.

## 4. Tagged DFA

A tagged DFA (TDFA) extends a standard DFA by attaching **tags** to transitions and states. Tags
allow the automaton to record semantic information during traversal, such as the start and end
positions of capture groups.

In LogSurgeon, TDFAs are used to extract sub-matches inside variables while preserving the
deterministic and linear-time properties of the DFAs.

### Tags and Registers in the DFA

Each capture group in a regex group is associated with a start and end tag. During TDFA traversal,
each tag corresponds to one or more registers:

- `final(tag)` - records the final value of the tag to be used post-lexing.
- `intermediate(tag,i)` - records transient information that may be forwarded into `final(tag)`.

At various DFA states register value are set or copied into other registers based on the register
action corresponding to the outgoing symbol. Once TDFA traversal is finished, the value in the
`final(tag)` register is used as the tag's value.

### Tagged Transitions

TDFA transitions are labeled like a standard DFA, but may also carry tag operations:

- **Positive tag operations** - applied when the transition is taken
- **Negative tag operations** - applied when a branch is rejected

At each step, tag operations update the corresponding registers. Multiple tag operations may occur
along a single transition.

### Match Semantics

A match succeeds if:

1. The traversal consumes the input and reaches an accepting state, and
2. Registers have been updated according to the taken transitions.

The TDFA reports both:

- **Match success** (which rules matched)
- **Capture variables** (via start/end positions stored in registers)

This enables subsequent stages, such as compression and search, to operate directly on the semantic
contents of the log message.

## 5. Compression Example

### Schema

```regex
delimiters: \n\r\t

tagged_user_id:user_id=(?<user_id>\d+)
tagged_session:session=(?<session>\w+\d+)
```

### TDFA

### Input

```text
My user_id=55 line.
```

### Execution

```text
States          | Input     | Action/Tag Updates     | Registers
-------------------------------------------------------------------------------
S0              | 'M'       | [fail]                 |
S0              | 'y'       | [fail]                 |
S0              | ' '       | [fail]                 |
S0              | 'u'       | -> S1                  |
S1              | 's'       | -> S2                  |
S2              | 'e'       | -> S3                  |
S3              | 'r'       | -> S4                  |
S4              | '_'       | -> S5                  |
S5              | 'i'       | -> S6                  |
S6              | 'd'       | -> S7                  |
S7              | '='       | -> S8                  |
S8              | '5'       | -> S9 + Set R2         | R2 = 8
S9              | '5'       | -> S10                 |
S10 [accepting] |           | Set R3, R2->R0, R3->R1 | R1 = R3 = 10, R3 = R2 = 8
S0              | ' '       | [fail]                 |
S0              | 'l'       | [fail]                 |
S0              | 'i'       | [fail]                 |
S0              | 'n'       | [fail]                 |
S0              | 'e'       | [fail]                 |
S0              | '.'       | [fail]                 |
```

### Result

```text
LogType:
  1. My user_id=<user_id> line.
Variables:
  1. user_id: 55
```

## 6. Search

The search problem is to find which logs match a given wildcard query. Logs are already compressed
into an archive with a logtype and variable dictionary, so the search algorithm compares the query
against these dictionaries to minimize archive decompression.

```
                     ------------------
                     | Wildcard Query |
                     ------------------
                             |
                             V
 ---------------------------------------------------------
 | Log Surgeon (LS)                                      |
 | - Parses query using schema                           |
 | - Generates interpretations using dynamic programming |
 |    - Static-text tokens                               |
 |    - Variable tokens: <variable>(value) (TDFA-based)  |
 ---------------------------------------------------------
                             |
                             V
              -------------------------------
              | CLP: generate subqueries    |
              | - Encoded variables         |
              | - Dictionary variables      |
              | - 2^k wildcard combinations |
              -------------------------------
                             |
                             V
        -------------------------------------------
        | CLP: compare against dictionaries       |
        | - Logtype dictionary                    |
        | - Variable dictionary                   |
        | - Only decompress archives with matches |
        -------------------------------------------
                             |
                             V
             ---------------------------------
             | CLP: grep decompressed logs   |
             | - Filtered by original query  |
             | - Output matched logs         |
             ---------------------------------
```


### A. Get Query Interpretations from Log Surgeon

Log Surgeon converts a query into **interpretations**, where each token may be:
- Static-text (literal),
- Variable (`<variable>(value)`) as determined by the schema and TDFA intersection.

It uses a **dynamic programming algorithm** over all the substrings of the query to minimize the
number ot TDFA intersections needed to generate the complete set of interpretations.

#### Schema Example:

```regex
delimiters: \n\r\t

int:-?\d+
float:-?\d+\.\d+
tagged_user_id:user_id=(?<user_id>\d+)
tagged_session:session=(?<session>\w+\d+)
```

#### Query Example:

```text
12* user_id=* 34.56 session=AB*
```

#### Interpretations Produced by Log Surgeon:

```text
12* user_id=* <float>(34.56) session=AB*
12* user_id=<user_id>(*)* <float>(34.56) session=AB*
12* user_id=* <float>(34.56) session=<session>(AB*)*
12* user_id=<user_id>(*)* <float>(34.56) session=<session>(AB*)*
<int>(12*) user_id=* <float>(34.56) session=AB*
<int>(12*) user_id=<user_id>(*)* <float>(34.56) session=AB*
<int>(12*) user_id=* <float>(34.56) session=<session>(AB*)*
<int>(12*) user_id=<user_id>(*)* <float>(34.56) session=<session>(AB*)*
<float>(12*) user_id=* <float>(34.56) session=AB*
<float>(12*) user_id=<user_id>(*)* <float>(34.56) session=AB*
<float>(12*) user_id=* <float>(34.56) session=<session>(AB*)*
<float>(12*) user_id=<user_id>(*)* <float>(34.56) session=<session>(AB*)*
```

### B. Generate Subqueries with Encoded Variables

For literal integer/float variables, the interpretation considers the variable to be encoded
directly into the archive if it is encodable (e.g., integer of 16 characters or fewer). However, if
the variables have a wildcard, CLP generates the **2^k combinations** of:
- **Encoded variables** (stored in the archive)
- **Dictionary variables** (stored in the variable dictionary)

#### Example subqueries for the query above:

```text
12* user_id=* <encoded>(34.56) session=AB*
12* user_id=<dict>(*)* <encoded>(34.56) session=AB*
12* user_id=* <encoded>(34.56) session=<dict>(AB*)*
12* user_id=<dict>(*)* <encoded>(34.56) session=<dict>(AB*)*
<dict>(12*)* user_id=* <encoded>(34.56) session=AB*
<dict>(12*)* user_id=<dict>(*)* <encoded>(34.56) session=AB*
<dict>(12*)* user_id=* <encoded>(34.56) session=<dict>(AB*)*
<dict>(12*)* user_id=<dict>(*)* <encoded>(34.56) session=<dict>(AB*)*
<encoded>(12*)* user_id=* <encoded>(34.56) session=AB*
<encoded>(12*)* user_id=<dict>(*)* <encoded>(34.56) session=AB*
<encoded>(12*)* user_id=* <encoded>(34.56) session=<dict>(AB*)*
<encoded>(12*)* user_id=<dict>(*)* <encoded>(34.56) session=<dict>(AB*)*
```

| Note: <dict> denotes a dictionary variable, <encoded> denotes an encoded variable.

### C. Validate Against Dictionaries

#### Logs:

```text
My log has user_id=55 34.56 session=AB23 and that's it.
123 user_id=22 34.56 session=AC45
123 user_id=41 34.56 session=AB11
123456789123456789 user_id=88 34.56 session=AB22
12a user_id=141 34.56 session=AB11

```

#### Logtype Dictionary:

```text
My log has user_id=<dict> <encoded> session=<dict> and that's it.
<dict> user_id=<dict> <encoded> session=<dict>
<encoded> user_id=<dict> <encoded> session=<dict>
12a user_id=<dict> <encoded> session=<dict>
```

#### Variable Dictionary:

```text
123456789123456789
22
41
55
AB11
AB23
AC45
```

#### Logtypes matching the subqueries:

```text
<dict> user_id=<dict> <encoded> session=<dict>
<encoded> user_id=<dict> <encoded> session=<dict>
12a user_id=<dict> <encoded> session=<dict>
```

- Each dictionary variable (`<dict>`) is validated against the variable dictionary.
- An Archive is only decompressed if at least one of the subqueries matches a logtype and all the
subqueries dictionary variables are in variable dictionary.

#### Decompressed Logs:

```text
123 user_id=22 34.56 session=AC45
123 user_id=41 34.56 session=AB11
123456789123456789 user_id=88 34.56 session=AB22
12a user_id=141 34.56 session=AB11
```

### D.Grep against the original query:

```text
123 user_id=41 34.56 session=AB11
123456789123456789 user_id=88 34.56 session=AB22
12a user_id=141 34.56 session=AB11
```
