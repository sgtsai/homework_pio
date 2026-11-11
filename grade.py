"""Trusted, deterministic Section 2.2 homework judge. Python 3.9+, stdlib only."""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import random
import re

MASK = 0xffffffff
# Each instruction has a fixed eight-byte teaching-machine representation:
# opcode, destination, source1, source2 (one byte each), immediate (four bytes).
FORMS = {
    'li': 'ri', 'add': 'rrr', 'addi': 'rri', 'and': 'rrr', 'andi': 'rri',
    'or': 'rrr', 'ori': 'rri', 'xor': 'rrr', 'xori': 'rri',
    'shl': 'rri', 'shr': 'rri', 'jnz': 'rl', 'jmp': 'l',
    'pull': '', 'out': 'ri', 'in': 'ri', 'push': '',
}


def assemble(text):
    labels, raw = {}, []
    for line_no, line in enumerate(text.splitlines(), 1):
        line = line.split('#', 1)[0].strip()
        if not line:
            continue
        if ':' in line:
            label, line = line.split(':', 1)
            label = label.strip()
            if not re.fullmatch(r'[A-Za-z_][A-Za-z_0-9]*', label) or label in labels:
                raise ValueError(f'line {line_no}: invalid or duplicate label')
            labels[label] = len(raw)
            line = line.strip()
        if line:
            raw.append((line_no, line.replace(',', ' ').split()))
    if not 1 <= len(raw) <= 64:
        raise ValueError('program must contain 1..64 instructions')
    result = []
    for line_no, tokens in raw:
        op, *args = tokens
        if op not in FORMS or len(args) != len(FORMS[op]):
            raise ValueError(f'line {line_no}: unknown opcode or operand count')
        parsed = []
        for kind, value in zip(FORMS[op], args):
            if kind == 'r':
                if not re.fullmatch(r'r[0-7]', value):
                    raise ValueError(f'line {line_no}: expected r0..r7')
                parsed.append(int(value[1]))
            elif kind == 'l':
                if value not in labels or labels[value] >= len(raw):
                    raise ValueError(f'line {line_no}: undefined/noninstruction label')
                parsed.append(labels[value])
            else:
                number = int(value, 0)
                if not -(1 << 31) <= number <= MASK:
                    raise ValueError(f'line {line_no}: immediate outside 32-bit range')
                parsed.append(number)
        if op in ('shl', 'shr') and not 0 <= parsed[2] <= 31:
            raise ValueError(f'line {line_no}: shift must be 0..31')
        if op in ('in', 'out') and not 1 <= parsed[1] <= 32:
            raise ValueError(f'line {line_no}: transfer must be 1..32 bits')
        result.append((op, parsed))
    return result


def expected(words):
    checksum, outputs = 0, []
    for word in words:
        # Independent declarative specification, not a target assembly solution.
        reverse = int(f'{word & 255:08b}'[::-1], 2)
        population = bin((word >> 8) & 255).count('1')
        sequence = (((word >> 16) & 255) + 1) & 255
        tag = ((word >> 24) & 255) ^ 0xa5
        transformed = (tag << 24) | (sequence << 16) | (population << 8) | reverse
        checksum = (((checksum << 5) | (checksum >> 27)) & MASK) ^ transformed
        outputs.extend((transformed, checksum))
    return outputs


def execute(program, words, schedule):
    """Return observed outputs, retired instructions, and elapsed ticks."""
    regs = [0] * 8
    tx, rx, outputs = deque(), deque(), []
    pc = osr = isr = in_count = sent = retired = 0
    out_count = 32
    limit = max(10000, 4096 * len(words))
    quiet = 0
    for tick in range(limit):
        if schedule == 'fast':
            feed = drain = True
        elif schedule == 'slow_tx':
            feed, drain = tick % 193 == 0, True
        elif schedule == 'slow_rx':
            feed, drain = True, tick % 127 == 0
        elif schedule == 'bursty':
            feed, drain = tick % 211 < 5, tick % 173 in (0, 1, 2)
        else:
            raise ValueError('unknown schedule')
        # Tick order is part of the published contract.
        if feed and sent < len(words) and len(tx) < 4:
            tx.append(words[sent])
            sent += 1
        if drain and rx:
            outputs.append(rx.popleft())
        if not 0 <= pc < len(program):
            raise ValueError('PC left program: use an explicit loop')
        op, a = program[pc]
        if (op == 'pull' and not tx) or (op == 'push' and len(rx) == 4):
            if op == 'pull' and sent == len(words) and not tx and not rx:
                quiet += 1
                if quiet == 64:
                    return outputs, retired, tick + 1
            else:
                quiet = 0
            continue
        quiet = 0
        nxt = pc + 1
        if op == 'li':
            regs[a[0]] = a[1] & MASK
        elif op in ('add', 'and', 'or', 'xor', 'addi', 'andi', 'ori', 'xori'):
            rd, rs, operand = a
            right = operand & MASK if op.endswith('i') else regs[operand]
            left = regs[rs]
            base = op[:-1] if op.endswith('i') else op
            value = {'add': lambda: left + right, 'and': lambda: left & right,
                     'or': lambda: left | right, 'xor': lambda: left ^ right}[base]()
            regs[rd] = value & MASK
        elif op == 'shl':
            regs[a[0]] = (regs[a[1]] << a[2]) & MASK
        elif op == 'shr':
            regs[a[0]] = regs[a[1]] >> a[2]
        elif op == 'jmp':
            nxt = a[0]
        elif op == 'jnz':
            if regs[a[0]] != 0:
                nxt = a[1]
        elif op == 'pull':
            osr, out_count = tx.popleft(), 0
        elif op == 'out':
            rd, count = a
            if out_count + count > 32:
                raise ValueError('OUT consumes more than one loaded word')
            regs[rd] = osr & ((1 << count) - 1)
            osr >>= count
            out_count += count
        elif op == 'in':
            rs, count = a
            if in_count + count > 32:
                raise ValueError('ISR overflow: PUSH before inserting more bits')
            isr = ((isr << count) | (regs[rs] & ((1 << count) - 1))) & MASK
            in_count += count
        elif op == 'push':
            if in_count != 32:
                raise ValueError('PUSH requires exactly 32 accumulated bits')
            rx.append(isr)
            isr = in_count = 0
        regs[0] = 0
        pc = nxt
        retired += 1
        if len(outputs) > 2 * len(words):
            raise ValueError('too many output words')
    raise ValueError('4096 ticks/input budget exceeded or program did not quiesce')


def suites():
    rng = random.Random(2202350)
    return [
        ('edge_values', [0, MASK, 0x80000000, 1, 0x12345678, 0xa5ff00ff], 'fast'),
        ('reverse_exhaustive', [0x12fe5500 | b for b in range(256)], 'fast'),
        ('popcount_exhaustive', [0x7fff00a5 | (b << 8) for b in range(256)], 'slow_tx'),
        ('header_wrap', [(tag << 24) | (seq << 16) | 0x8142
                         for tag in range(256) for seq in (0, 127, 254, 255)], 'fast'),
        ('backpressure', [rng.getrandbits(32) for _ in range(1024)], 'slow_rx'),
        ('long_stream', [rng.getrandbits(32) for _ in range(4096)], 'bursty'),
    ]


def grade(path):
    text = path.read_text(encoding='utf-8')
    program = assemble(text)
    rows = []
    for name, words, schedule in suites():
        try:
            result, retired, ticks = execute(program, words, schedule)
            target = expected(words)
            ok = result == target
            mismatch = next((i for i, (a, b) in enumerate(zip(result, target)) if a != b), None)
            rows.append(dict(suite=name, passed=ok, points=10 if ok else 0,
                             inputs=len(words), output_words=len(result),
                             retired=retired, ticks=ticks, first_mismatch=mismatch))
        except ValueError as error:
            rows.append(dict(suite=name, passed=False, points=0, error=str(error)))
    correct = all(row['passed'] for row in rows)
    cycles = None
    cycle_points = memory_points = 0
    memory = 96 + 8 * len(program)
    performance_error = None
    if correct:
        rng = random.Random(22099)
        words = [rng.getrandbits(32) for _ in range(8192)]
        try:
            result, retired, _ = execute(program, words, 'fast')
        except ValueError as error:
            performance_error = str(error)
            result = None
        if result == expected(words):
            cycles = retired / len(words)
            cycle_points = 20 if retired <= 100 * len(words) else 15 if retired <= 150 * len(words) else 8 if retired <= 250 * len(words) else 0
            memory_points = 20 if memory <= 352 else 12 if memory <= 480 else 5
    return dict(program=str(path), sha256=hashlib.sha256(text.encode()).hexdigest(),
                instructions=len(program), simulated_memory_bytes=memory,
                suites=rows, correctness_points=sum(row['points'] for row in rows),
                cycles_per_input=cycles, cycle_points=cycle_points, memory_points=memory_points,
                performance_error=performance_error,
                total=sum(row['points'] for row in rows) + cycle_points + memory_points)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('program', nargs='?', default='submission.asm')
    parser.add_argument('--json', action='store_true', help='print machine-readable results')
    args = parser.parse_args()
    try:
        result = grade(Path(args.program))
    except (ValueError, OSError) as error:
        print(json.dumps(dict(total=0, error=str(error)), indent=2))
        return 1
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for row in result['suites']:
            print(f"{row['suite']:22} {row['points']:2}/10  {row.get('error', 'PASS' if row['passed'] else 'FAIL')}")
        print(f"Instructions: {result['instructions']}; model memory: {result['simulated_memory_bytes']} bytes")
        print(f"Retired cycles/input: {result['cycles_per_input']}")
        print(f"Correctness {result['correctness_points']}/60; cycles {result['cycle_points']}/20; memory {result['memory_points']}/20")
        print(f"TOTAL: {result['total']}/100")
    return 0 if result['total'] >= 60 else 1


if __name__ == '__main__':
    raise SystemExit(main())
