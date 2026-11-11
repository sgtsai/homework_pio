"""Tests for the trusted judge; these do not solve the homework."""
import unittest
from grade import assemble, execute, expected


class JudgeTests(unittest.TestCase):
    def test_published_examples(self):
        self.assertEqual(expected([0]), [0xa5010000] * 2)
        self.assertEqual(expected([0xffffffff]), [0x5a0008ff] * 2)
        self.assertEqual(expected([0x12345678]), [0xb735041e] * 2)
        self.assertEqual(expected([0, 0]), [0xa5010000] * 3 + [0x05210014])

    def test_double_echo_all_schedules(self):
        program = assemble('start: pull\nout r1,32\nin r1,32\npush\nin r1,32\npush\njmp start')
        words = list(range(40)) + [0xffffffff]
        for schedule in ('fast', 'slow_tx', 'slow_rx', 'bursty'):
            actual, retired, _ = execute(program, words, schedule)
            self.assertEqual(actual, [w for w in words for _ in range(2)])
            self.assertEqual(retired, 7 * len(words))

    def test_partial_shift_transfers(self):
        p = assemble('start: pull\nout r1,8\nout r2,24\nin r1,8\nin r2,24\npush\njmp start')
        self.assertEqual(execute(p, [0x12345678], 'fast')[0], [0x78123456])

    def test_zero_register_and_wrap(self):
        p = assemble('start: pull\nli r0,99\naddi r1,r0,-1\naddi r1,r1,2\nin r1,32\npush\njmp start')
        self.assertEqual(execute(p, [0], 'fast')[0], [1])

    def test_reject_invalid_programs(self):
        for text in ('', 'li r8,0', 'shl r1,r2,32', 'jmp missing', 'pull\n' * 65):
            with self.assertRaises(ValueError):
                assemble(text)

    def test_runtime_guards(self):
        for text in ('out r1,1', 'pull\npush', 'pull\nout r1,32\nout r1,1',
                     'li r1,0\nin r1,32\nin r1,1', 'forever: jmp forever'):
            with self.assertRaises(ValueError):
                execute(assemble(text), [1], 'fast')


if __name__ == '__main__':
    unittest.main()
