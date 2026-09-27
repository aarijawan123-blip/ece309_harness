# Project 2 Design Log

## Scope and structure

Project 2 adds Message, Conversation, and SentinelScanner to the supplied
C++17 harness. The model clients, CLI, execution loop, and CMake configuration
remain unchanged. Message owns a string and defaults to an empty System message,
so Conversation can allocate arrays of default-constructed objects. Accessors
return the role and a const reference to content. Implementation and testing
were completed with AI assistance in separate commits for each project part.

## Growth factor and amortized cost

Conversation starts with a null pointer, zero size, and zero capacity. Its first
append allocates one slot. Later full arrays double their capacity, giving
1, 2, 4, 8, and so on. Doubling is simple to explain and avoids reallocating
on every insertion. A capacity check prevents multiplication overflow.

For n positive appends starting from empty, let C be the final capacity. Then
n <= C < 2n. The arrays relocated during growth contain 1 + 2 + 4 + ... + C/2
messages, totaling C - 1 < 2n. Default construction of new slots totals
1 + 2 + ... + C = 2C - 1 < 4n, and destruction of old slots is also linear.
Adding the n insertions therefore gives O(n) total container work, or amortized
O(1) per append. Constructing a message from text has a separate cost depending
on its length; the bound does not claim arbitrary strings are free to copy.

Nothing is evicted. A System message is accepted only at position zero; a later
System message throws invalid_argument. Invalid at() indices throw out_of_range.
Growth invalidates pointers into the previous array.

## Rule of Five and exception safety

Conversation exclusively owns its Message array. The destructor releases it
with delete[]. Copy construction allocates another array and copies each
message, giving independent storage. If a string copy throws, the constructor
catches the exception, deletes its new array, and rethrows. This matters because
a failed constructor does not run that object's destructor.

Copy assignment first constructs a temporary copy. Only after that succeeds
does move assignment replace the destination, leaving the original unchanged
if copying fails. Move construction and assignment transfer the pointer, size,
and capacity, then reset the source to null and zero. Move assignment releases
the destination's old array. Both assignments handle self-assignment, and
moved-from objects can be reused or destroyed.

Growth allocates before changing the old array and then uses Message's
nonthrowing move assignment. Passing append's argument by value protects a
message taken from the same array during reallocation. Empty end() avoids
pointer arithmetic on null. Tests check independent array and long-string
storage, transferred pointer identity, source reset, reuse, self-assignment,
and growth through 4,097 messages. Allocation-failure handling was reviewed
in the code; artificial allocation failures were not injected.

## Sentinel buffer bound

For a nonempty sentinel of length m, feed searches pending text plus the next
chunk. Without a match, it emits all but the final min(text.size(), m-1) bytes.
A future sentinel crossing the boundary can use at most m-1 previous bytes;
a complete m-byte match would already have been detected. Thus the emitted
prefix cannot be needed for a later match.

Initially pending is empty. Each unsuccessful feed assigns at most m-1 bytes;
a successful feed or flush clears it. Induction establishes pending.size()
<= m-1 after every operation. Whole chunks are never appended into pending
itself. Retained scanner state is O(m), constant for the assignment's fixed
sentinel. Temporary combined text and returned output use O(chunk size + m)
space; this is not a constant-space claim for arbitrarily large chunks.

The first match emits only preceding text and stops the scanner. Later text
is discarded. Without a match, flush releases the incomplete suffix. Empty
sentinels are rejected. Tests cover every split boundary, one-byte chunks,
false matches, overlapping patterns, and the pending bound over 4 MiB.

## Validation and hindsight

Debug and Release builds passed 26 C++ test groups and three CLI checks with
warnings treated as errors, AddressSanitizer, UndefinedBehaviorSanitizer, and
leak detection. Replay tests compare messages, roles, output, and stop reasons;
CLI checks verify saved transcripts after sentinel, EOF, and turn-limit exits.
No sanitizer errors were reported. GCC rejected the intentional self-move test;
a helper now exercises it without disabling warnings.

In hindsight, shared test functions would have been useful from the beginning.
Separate component runners enabled early checks, but integrating them later
required moving their checks into headers. Keeping shared checks and thin
runners from the start would reduce that reorganization while retaining useful
independent builds. Earlier working notes remain available in Git history.
