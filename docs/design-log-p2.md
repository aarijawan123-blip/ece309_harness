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



## What I would change differently
