#!/usr/bin/env python3

import email
import pathlib
import subprocess
import unittest
import sys


class ConvertError(Exception):
    def __init__(self, message):
        Exception.__init__(self)
        self.message = str(message)


def convert_to_memory(input_path):
    args = (
        './archive-to-multipart',
        str(input_path),
        '{"filename":"archive.ext/FILENAME","metadata":{"foo":"bar"}}',
        'MIME-BOUNDARY'
    )

    with subprocess.Popen(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE
    ) as process:
        (out, err) = process.communicate()

        if process.returncode != 0:
            raise ConvertError(err)

        return out


def iter_test_dir_expected_paths(path):
    json_blob_pairs = zip(
        sorted(path.glob('*.json')),
        sorted(path.glob('*.blob'))
    )

    last = path.glob('[de]*')

    for pair in json_blob_pairs:
        yield (pair[0].name, pair[0].read_bytes())
        yield (pair[1].name, pair[1].read_bytes())

    for item in last:
        yield (item.name, item.read_bytes())


def iter_multipart_form_data_bytes(b):
    message = email.message_from_bytes(
        b'Content-Type: multipart/form-data; boundary=MIME-BOUNDARY\r\n\r\n'
        + b
    )

    parts = message.walk()
    next(parts)  # ignore the container

    for part in parts:
        name_q = part['Content-Disposition'].replace('form-data; name=', '')
        name = name_q[1:-1]  # nix quotation marks
        yield (name, part.get_payload())


class TestDirTestCase(unittest.TestCase):
    def __init__(self, path):
        unittest.TestCase.__init__(self)
        self.path = path

    def runTest(self):
        out = convert_to_memory(self.path / 'input.blob')

        actual_parts = filter(
            lambda x: x[0] != 'progress',
            iter_multipart_form_data_bytes(out)
        )

        expected_parts = iter_test_dir_expected_paths(self.path)

        for (actual, expected) in zip(actual_parts, expected_parts):
            actual_name = actual[0]
            expected_name = expected[0]
            self.assertEqual(actual_name, expected_name)

            actual_contents = actual[1]
            expected_contents = expected[1].decode()
            self.assertEqual(actual_contents, expected_contents)

        # Make sure we don't have any stray parts
        self.assertEqual(list(actual_parts), [])
        self.assertEqual(list(expected_parts), [])


def suite():
    ret = unittest.TestSuite()
    for path in pathlib.Path('test').glob('test-*'):
        ret.addTest(TestDirTestCase(path))
    return ret


if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    result = runner.run(suite())
    if not result.wasSuccessful():
        sys.exit(1)
