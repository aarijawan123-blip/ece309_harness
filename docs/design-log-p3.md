# Project 3 Design Log (in progress)

**P2 code used:** our own Conversation and SentinelScanner, with
USE_P2_REFERENCE=OFF. **Tools used:** Codex for specification review, starter
migration, registry and workspace implementation, tests, and these draft notes; GCC 15.2 and
CMake in WSL Ubuntu for compilation; ASan and UBSan for runtime checking.
This is a checkpoint for parts 1–4, not the final 500–800-word submission log.

## Hash table

The registry uses separate chaining. Each element in a vector is a linked list
of entries containing a stored name and a unique_ptr<Tool>. This makes a
collision straightforward: search the chosen list and compare the full names.
There is no deletion operation to support, and chaining avoids the wraparound
and empty-slot rules of linear probing.

The hash follows the FNV-1a-style example in Appendix A, including its supplied
starting constant. Each byte is XORed into a 64-bit unsigned value and multiplied
by the supplied prime. The bucket index is the resulting hash modulo the bucket
count. The test suite checks order sensitivity, changes at every position in a
long name, and at least 9,990 distinct full hashes among 10,000 names.

The initial bucket count is 16. Before a new unique entry would exceed load 1.0,
the table doubles. Duplicate and null additions are rejected before resizing.
With well-distributed hashes, ordinary insertion and lookup take expected O(1)
time for bounded-length names. A resize takes O(n), but doubling leaves room
for many ordinary insertions before the next resize. Spreading that cost over
those insertions gives amortized expected O(1) insertion. Hashing itself takes
O(k) for a name of length k; a badly colliding bucket can still require O(n)
comparisons.

Measured on Linux with sanitizers, inserting tool_0 through tool_9999:

| Measurement | Registry hash | Weak hash s[0] + s.size() |
|---|---:|---:|
| Buckets | 16,384 | 16,384 |
| Load factor | 0.610352 | 0.610352 |
| Longest chain | 6 | 9,000 |

The weak hash gives all four-digit suffixes the same bucket because they have
the same first character and length. Low overall load alone cannot repair that.

## Pointer stability and ownership

Each tool is allocated separately and owned by exactly one unique_ptr. Rehash
allocates a new bucket vector first, then uses list::splice to transfer nodes
into their new buckets without reallocating the tools. Returned Tool pointers
therefore keep their addresses. The scale test saves and checks every tool's
pointer. Ownership tests count destruction on duplicate rejection and registry
cleanup after multiple resizes. The caller must never delete a find result.

## Verification and development record

The requested scope was: "Can you do parts 1-3 as a single commit". The four
P2 container/scanner files were retained, with the two source files relocated
to src/core. The provided P3 Message and infrastructure replaced their P2
versions. Original P2 tests and README were archived. The separately attached
specification supplies the deadline missing from the ZIP's copy.

The migrated P2 regression suite passed 14/14 tests. After registry development,
13/13 registry tests passed, including forced collisions, duplicate ownership,
resizing at the threshold, iteration, and the 10,000-entry comparison. The
--tools output and --no-tools greeting matched the supplied golden files.
The build used warnings as errors and ASan/UBSan; no compiler or runtime errors
were found in those checks. An editing-tool patch was initially rejected for
listing the same file twice; it applied no changes and was resubmitted correctly.
An early read also targeted not-yet-extracted paths; extraction resolved that.
Neither was a compiler or program failure.

## Remaining sections

Calculator evaluation is still the provided stub. Its tests and full
integration testing belong to later parts. Add the
calculator stack trace, errors-as-values discussion, and hindsight section
when those parts are complete, then revise this draft to the required word count.

Git's whitespace check also identified five trailing-whitespace lines in the
unchanged provided model files; those were preserved with the starter. Added
LF attributes for CLI fixtures so Windows checkout settings cannot change
the expected output or inputs. Authored files pass the whitespace check.


## Part 4: workspace resolution

The request was to start and commit part 4 separately. The resolver follows
section 4.4 in order, checking empty input and embedded NUL bytes before
constructing a filesystem path. It rejects rooted input, normalizes the
relative path, and rejects a leading parent component. This accepts legal
names containing two dots without allowing an escape. A normalized dot returns
the root itself. No filesystem operations are used in resolve, so nonexistent
files and roots are acceptable. Path-processing exceptions become a false
result and an error message.

The new tests cover every row of the required table, with exact resolved paths
for accepted inputs and explanations for rejected ones. Additional checks cover
NUL bytes at either end, absolute paths inside the root, bounded string views,
nonexistent roots, and the separation between lexical resolution and the
provided symlink check. A real file-tool round trip writes, reads, and lists a
file in a temporary directory; escape attempts leave an outside file unchanged.

Linux testing passed all 11 workspace tests, 13 registry tests, 14 P2 regression
tests, and the provided escape-attempt tool-loop test. The escape-attempt CLI
session matched its supplied golden output after the same error-message
normalization used by the provided test script. The build used warnings as
errors, AddressSanitizer with leak detection, and UndefinedBehaviorSanitizer.
No compiler warnings, test failures, or sanitizer findings occurred in part 4.
The calculator and provided production files were left unchanged.
