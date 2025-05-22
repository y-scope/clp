# Search KV-IR streams

In this section, we will talk about the algorithm and design for searching KV-IR stream. Searching
a KV-IR stream is similar to searching a clp-s archive: we allow users to search an KV-IR stream
using the same query syntax (TODO: add a reference). From a high-level, searching a KV-IR stream
includes the following steps:

1. Pre-process the query AST.
2. Initialize partial resolutions of AST column descriptors.
3. Update partial resolutions on each deserialized new schema tree node IR unit.
4. Evaluate the query on each deserialized log event IR unit.



