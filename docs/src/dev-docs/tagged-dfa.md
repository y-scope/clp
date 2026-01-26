# Tagged Deterministic Finite Automata

This document explains the inner workings of the deterministic finite automaton (DFA) implemented in
LogSurgeon and used for compression and search in CLP.

## Contents

- [Schema](#1-schema)
- [NFA](#2-nfa)
- [DFA](#3-dfa)
- [Tagged DFA](#4-tagged-dfa)
- [Compression](#5-compression-example)
- [Search](#6-search-example)

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

Each regex rule is used to construct an NFA, which is eventually considered durign DFA construction.

### Priority and Ordering

When multiple regex rules match the same input, the ordering defined in the schema is used to
determine priority.

### Example Schema
```regex
delimiters: \n\r\t

float:-?\d+\.\d+
int:-?\d+
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
- Literal character produce linear sequences of states.
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
reasons NFAs are inefficient for traversal. For runtime performance, an NFA must be converted to
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

### Capture Groups in Regex

Above, we i

### Tagged NFA



### Tags and Registers in the DFA

Each capture group in a regex group is associated with a start and end tag. During TDFA traversal,
each tag corresponds to one or more registers:

- `final(tag)` - records the final value of the tag to be used post-lexing.
- `intermediate(tag,i)` - records transient information that may be forwarded into `final(tag)`.

At various DFA states register value are set or copied into other registers based on the register
action corresponding to the out-going symbol. Once TDFA traversal is finished, the value in the
`final(tag)` register is used as the tag's value.

### Tagged Transitions

TDFA transitions are labeled like a standard DFA, but may also carry tag operations:

- **Positive tag operations** - applied when the transition is taken
- **Negative tag operations** - applied when a branch is rejected

At each step, tag operations update the corresponding registers. Multiple tag operations may occur
along a single transition.

### Ambiguity and Leftmost-Greedy Resolution



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

## 6. Search Example

### Schema

```text
delimiters: \n\r\t

tagged_user_id:user_id=(?<user_id>\d+)
tagged_session:session=(?<session>\w+\d+)
```

### Query

```text
user_id=** session=AB**
```

### Logs

```text
My log has user_id=55 session=AB23 and thats it.
user_id=22 session=AC45
user_id=41 session=AB11
```

### Logtype Dictionary

```text
My log has user_id=<user_id> session=<session> and thats it.
user_id=<user_id> session=<session>
```

### Variable Dictionary

```text
22
41
55
AB11
AB23
AC45
```


### Normalize Query

```text
user_id=* session=AB*
```

### Form All Interpretations

```text
<user_id=* session=AB*>
<user_id=*> <session=AB*>
```

### Intersect DFA Of Each Interpretation Token with Schema DFA

```text
<user_id=* session=AB*> -> no match
<user_id=*>             -> can match a `tagged_user_id`
<session=AB*>           -> can match a `tagged_session`
```

### Subquery

```text
1. user_id=* session=AB*
2. user_id=<user_id>* session=AB*
3. user_id=* session=<session>*
4. user_id=<user_id>* session=<session>*
```

### Compare Against Logtype Dictionary

```text
user_id=<user_id> session=<session> -> matches subquery (4)
```


### Compare Against Variable Dictionary

```text
*   -> matches full dictionary
AB* -> matches AB11, AB23
```

### Decompress Logtypes With the Above Form to Get Potential Matches

```text
user_id=22 session=AC45
user_id=41 session=AB11
```

### Grep Potential Matches with Original Query

```text
user_id=41 session=AB11
```
