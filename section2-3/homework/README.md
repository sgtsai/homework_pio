# Section 2.3: Safe Asynchronous Transfer Queue

Read [homework.txt](homework.txt) for the complete contract and scoring rules.
Implement [submission.c](submission.c) and [submission.h](submission.h), then run:

```sh
python grade.py
python grade.py --json > result.json
```

Build a capacity-eight queue with blocking producers/consumers, safe shutdown,
validated copy/fill jobs, and worker threads that transfer outside the queue lock.

Scoring: **60 correctness + 20 independent progress + 20 queue memory**.
The grader uses a 20-second hang limit per run; raw execution speed does not
earn points. Read the source-validity rules in the assignment before grading.

The starter compiles but intentionally scores zero. No complete solution is
included. Python 3.9+, a C11 compiler, and POSIX threads are required. Linux/WSL
is a suitable environment. Builds stay in the ignored `build/` directory.

Background: [Section 2.3 tutorial](../tutorial.txt).

Validation: the starter compiled and scored 0/100. A separate in-memory
implementation passed all six correctness suites and the independent-progress
test, including repeated threaded runs. Its normalized queue storage was
480 bytes: 448 bytes for eight descriptors plus 32 bytes overhead, satisfying
the full memory-score tier. No reference solution source is included.
