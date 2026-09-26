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



## Rule of Five evidence



## Sentinel scanner: bounded pending_ proof



## What I would change differently
