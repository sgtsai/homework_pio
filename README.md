# Systems Programming Homework: Sections 2.2 and 2.3

- **Section 2.2:** [Bitstream Packet Processor](homework.txt), in this repository's root directory.
- **Section 2.3:** [Safe Asynchronous Transfer Queue](section2-3/homework/homework.txt), with its own incomplete C starter and grader.
- **Pthread syntax guide:** Section 13 of the [Section 2.3 assignment](section2-3/homework/homework.txt) explains mutexes, condition variables, notifications, thread creation/joining, and pointer pitfalls.

To run Section 2.3:

```sh
cd section2-3/homework
python grade.py
```

## Section 2.2: Bitstream Packet Processor

Read [homework.txt](homework.txt) for the complete assignment, machine specification, examples, and fixed grading rubric.

Background material and the original demos are in [reading/tutorial.txt](reading/tutorial.txt).

Implement [submission.asm](submission.asm), complete [report_template.txt](report_template.txt) as `report.txt`, and run:

```sh
python grade.py submission.asm
python grade.py submission.asm --json > result.json
```

The score is 60 points for correctness, 20 for simulated execution cycles, and 20 for simulated target memory. Efficiency points require all correctness checks and the performance stream to pass. The supplied starter intentionally scores zero.

This is a custom teaching machine inspired by the section's bitwise, RISC-V, and PIO concepts; it is not hardware-compatible assembly. Python 3.9+ is sufficient, with no additional packages.

Run `python selftest.py` to check the judge. During preparation, all six judge self-tests passed, and a separate feasibility solution passed every grading suite at **32 instructions, 352 bytes, and 95 cycles per input**, earning **100/100**. The solution is not included in the student starter. Slow-input, slow-output, and bursty schedules were also verified to cause substantial waiting without changing correct results.
