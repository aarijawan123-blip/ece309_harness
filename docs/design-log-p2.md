# Design Log — Project 2

(500–800 words total. See spec §5 for what each section must cover.)

## Part 2 working notes

Message is implemented entirely in its header because its constructors and
accessors are short. The default constructor creates an empty System message,
allowing Conversation to allocate arrays of Message objects later. The other
constructor accepts a string by value and moves it into the owned content
member. Changing the caller's string therefore cannot change the stored text.
The accessors are const and noexcept; content returns a const reference to avoid
copying. Standard string ownership handles cleanup without raw allocation or
custom copy/move operations in Message. Standalone checks allow this part to be
validated before Conversation and SentinelScanner exist. These working notes
will be incorporated into the final 500-800-word log in Part 6.

Validation: the initial compile could not run because WSL lacked g++. After
installing the Ubuntu g++ package, the standalone checks compiled with C++17,
strict warnings treated as errors, and AddressSanitizer/UndefinedBehaviorSanitizer.
The program printed `Message checks passed.` with leak detection enabled and
no compiler or sanitizer diagnostics. No implementation corrections were needed.
The complete CMake build awaits the remaining core classes.

## Growth factor and amortized cost

Part 3 uses capacity 0 initially, then 1, 2, 4, 8, and so on. For n appends,
let C be the final capacity. For n > 0, C < 2n (including C = n = 1).
Growth relocates 1 + 2 + ... + C/2 = C - 1 < 2n messages in total.
Default construction of allocated slots also totals 1 + 2 + ... + C < 4n;
destruction of old arrays is another linear total. Together with n insertions,
container work is O(n), hence amortized O(1) per append. String construction
from caller text has its own length-dependent cost; this claim concerns the
container's storage operations. A capacity check prevents doubling overflow.


## Rule of Five evidence

Part 3 owns one Message array. The destructor uses delete[]. Copy construction
allocates independent storage and copies messages, releasing the new array
if a string copy throws. Copy assignment builds a temporary before replacing
the destination, so failed copying leaves it unchanged. Moves transfer the
pointer, size, and capacity and reset the source to null/zero without copying
elements. Assignment checks for self-assignment. Array growth allocates first,
then uses Message's nonthrowing move assignment; allocation failure leaves the
existing conversation intact. append takes its argument by value, which also
protects an existing element passed as input during growth. Empty end() avoids
arithmetic on a null pointer. Bounds errors throw std::out_of_range; late System
messages throw std::invalid_argument so a System message remains first.

Standalone tests inspect actual capacity through a private friend, confirm
distinct array and long-string storage after copying, check pointer identity
after moving, reuse moved-from objects, and exercise populated destinations
and self-assignment. Allocation-failure paths are reviewed structurally rather
than tested by artificially exhausting system memory.

Part 3 validation: GCC initially rejected the deliberate self-move expression
under -Werror=self-move. The test now passes both references to a small move
assignment helper, exercising the same case without disabling diagnostics.
All eight Conversation test groups and the existing Message checks then passed
with C++17, strict warnings as errors, AddressSanitizer, UndefinedBehaviorSanitizer,
and leak detection enabled. No memory errors were reported. The provided
CMake file and harness/model implementations remain unchanged; full integration
awaits SentinelScanner.


## Sentinel scanner: bounded pending_ proof

Part 4 follows the specified trailing-window algorithm. For a nonempty sentinel
of length m, feed searches the previous pending text plus the current chunk.
If no match exists, it emits everything except the last min(text.size(), m-1)
bytes. Any future match crossing this boundary can use at most m-1 old bytes;
a full m-byte match would already have been detected. Thus emitting the prefix
cannot lose a future match. Initially pending is empty. Each unsuccessful feed
assigns at most m-1 bytes, and a successful feed or flush clears it. Induction
therefore gives pending.size() <= m-1 after every operation. The implementation
never appends a whole chunk into pending itself.

The first match returns only its preceding text and permanently marks the
scanner stopped. Later input is ignored, which also handles calls after a match
at an earlier split point. Without a match, flush releases an incomplete suffix
as ordinary text. Empty sentinels are rejected to avoid subtracting one from
zero. Retained state is O(m), constant for the fixed assignment sentinel;
temporary combined text and returned output require O(chunk size + m) storage.
This distinction avoids claiming constant total memory for arbitrarily large
chunks. For a fixed sentinel, processing does not repeatedly search an ever-
growing reply. Tests check every split point, overlapping patterns, and the
pending bound throughout 4 MiB delivered one byte at a time.

Part 4 validation: all nine scanner test groups passed with strict GCC warnings
treated as errors, AddressSanitizer, UndefinedBehaviorSanitizer, and leak
detection. No scanner code correction was needed. WSL lacked CMake, so CMake
and its dependencies were installed. Both supplied CMake targets then built
without diagnostics, and the existing Message and Conversation checks passed.
The greeting script stopped after three turns, hid the sentinel from terminal
output, and retained it in the saved transcript. This was a smoke check;
test_p2 remains the provided empty test placeholder until Part 5.


## Part 5 integration notes

The main test_p2 runner now includes all component checks plus eight integration
groups using the provided ScriptedModelClient, ReplayModelClient, and Harness.
Component checks moved into shared test headers; their original standalone
runners still work. This keeps the supplied CMake file unchanged. Assertions
are explicitly enabled in test translation units even under Release builds.

Integration tests cover zero/two/default-20 turn limits, system-message ordering,
sentinel halt with chunk sizes from one byte upward, discarded post-sentinel
text, EOF, blank lines, and exhaustion of both clients. Transcript round trips
save a conversation to a temporary file and compare all roles, content, output,
and stop reasons after replay, with both sentinel and EOF endings. The test
writer is separate from main.cpp, so an additional Bash script checks the actual
CLI's saved transcript and complete output on sentinel, EOF, and turn-limit
shutdown. Test fixtures use temporary files rather than overwriting user data.

Validation: Debug and Release CMake builds completed with -Werror and the
provided warning/sanitizer flags. Each build passed all 26 C++ test groups and
all three CLI checks with AddressSanitizer, UndefinedBehaviorSanitizer, and leak
detection enabled. No compiler or runtime test failures occurred in Part 5,
and no production source or provided CMake changes were needed. The design log
is still working notes; Part 6 will condense it to the required 500-800 words.

## What I would change differently
