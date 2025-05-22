# Search KV-IR streams

In this section, we will talk about the algorithm and design for searching KV-IR stream.

To understand KV-IR search, we need to review how clp-s handles search queries. clp-s allows users
to search a compressed archive using a KQL (TODO: reference) query. The query contains one or more
filters chaining with logical operators. Each filter is essentially a key-value pair with a filter
operator, where the key is the queries column, the value is the queried value, and the operator can
be an exact match or numeric comparators. Notice we allow both the keys and the values to contain
`*` wildcards.

Internally, clp-s first parses the KQL query into
an abstract syntax tree (AST), and then perform a number of optimizations to determine the matchable
schemas and dictionary values. Finally, clp-s decompresses relevant ERTs (TODO: reference) and search through each
record by evaluating against the AST.

KV-IR stream search uses the same search syntax to accept user queries, but comparing to clp-s archive
search, it has the following fundamental differences:
- We don't have the merged schema tree ready at the beginning of the search since schema tree is
  dynamically constructed when reading through the stream. This means the queried keys, especially
  those keys with wildcards, can only be resolved to concrete schema tree nodes during the stream
  deserialization.
- The IR stream serialize log events in sequential order without grouping them by the schema. This
  means every log event in the stream must be deserialized and evaluated against the search AST.

From a high-level, searching a KV-IR stream includes the following steps:

1. Pre-process the query AST.
2. Initialize partial resolutions of AST column descriptors.
3. Update partial resolutions on each deserialized new schema tree node IR unit.
4. Evaluate the query on each deserialized log event IR unit.



