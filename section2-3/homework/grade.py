"""Section 2.3 public grader: Python 3.9+, C11 compiler, POSIX threads."""
import argparse
import json
import os
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parent
SUITES = ('copy_modes', 'validation', 'fifo', 'closing', 'concurrent', 'pipeline')


def run_case(executable, name):
    try:
        p = subprocess.run([str(executable), name], cwd=ROOT,
                           capture_output=True, text=True, encoding='utf-8',
                           errors='replace', timeout=20)
        return p.returncode == 0 and p.stdout.strip() == 'PASS', (p.stderr or p.stdout).strip()
    except subprocess.TimeoutExpired:
        return False, 'Exceeded the 20-second hang-detection limit'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', default=os.environ.get('CC', 'cc'), help='compiler executable (no flags)')
    parser.add_argument('--json', action='store_true')
    parser.add_argument('--tsan', action='store_true', help='optional ThreadSanitizer diagnostic run; not official scoring')
    args = parser.parse_args()
    build = ROOT / 'build'
    build.mkdir(exist_ok=True)
    executable = build / ('judge.exe' if os.name == 'nt' else 'judge')
    flags = ['-std=c11', '-O2', '-Wall', '-Wextra', '-pthread']
    if args.tsan:
        flags = ['-std=c11', '-O1', '-g', '-Wall', '-Wextra', '-pthread', '-fsanitize=thread']
    # The compiler already runs in ROOT. Keep non-ASCII parent directory names
    # out of arguments passed between Windows GCC and its linker.
    command = [args.cc, *flags, 'submission.c', 'judge.c', '-o',
               (Path('build') / executable.name).as_posix()]
    try:
        compilation = subprocess.run(command, cwd=ROOT, capture_output=True,
                                     text=True, encoding='utf-8', errors='replace', timeout=60)
    except (OSError, subprocess.TimeoutExpired) as error:
        print(json.dumps({'total': 0, 'build_error': str(error)}, indent=2))
        return 1
    if compilation.returncode:
        print(json.dumps({'total': 0, 'build_error': compilation.stderr}, indent=2))
        return 1
    rows = []
    for name in SUITES:
        # Repeat concurrent cases; still not a proof of absence of data races.
        repetitions = 5 if name in ('closing', 'concurrent', 'pipeline') else 1
        ok, detail = True, ''
        for _ in range(repetitions):
            ok, detail = run_case(executable, name)
            if not ok:
                break
        rows.append({'suite': name, 'passed': ok, 'points': 10 if ok else 0, 'detail': detail})
    all_correct = all(row['passed'] for row in rows)
    progress_ok, progress_detail = run_case(executable, 'progress') if all_correct else (False, 'Requires all correctness suites')
    progress_points = 20 if progress_ok else 0
    memory_points, memory, storage = 0, None, None
    if all_correct:
        try:
            m = subprocess.run([str(executable), 'memory'], cwd=ROOT,
                               capture_output=True, text=True, encoding='utf-8',
                               errors='replace', timeout=20, check=True)
            memory, storage = map(int, m.stdout.split())
            overhead = memory - storage
            if overhead >= 0:
                memory_points = 20 if overhead <= 64 else 12 if overhead <= 128 else 5 if overhead <= 256 else 0
        except (ValueError, subprocess.SubprocessError):
            pass
    score = sum(row['points'] for row in rows) + progress_points + memory_points
    result = {'mode': 'diagnostic-tsan' if args.tsan else 'official', 'suites': rows,
              'progress_points': progress_points, 'progress_detail': progress_detail,
              'normalized_queue_bytes': memory, 'eight_job_bytes': storage,
              'memory_points': memory_points, 'total': score}
    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for row in rows:
            print(f"{row['suite']:14} {row['points']:2}/10  {'PASS' if row['passed'] else row['detail']}")
        print(f'Independent progress: {progress_points}/20; {progress_detail}')
        print(f'Queue memory: {memory_points}/20; normalized bytes={memory}, eight-job storage={storage}')
        print(f'TOTAL: {score}/100' + (' (sanitizer diagnostic only)' if args.tsan else ''))
    return 0 if score >= 60 else 1


if __name__ == '__main__':
    raise SystemExit(main())
