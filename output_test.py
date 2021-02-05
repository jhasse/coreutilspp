#!/usr/bin/env python3

"""Runs ./build/rm++ and checks if the output is correct.

In order to simulate a smart terminal it uses the 'script' command.
"""

import os
import platform
import subprocess
import sys
import tempfile
import unittest

default_env = dict(os.environ)
if 'CLICOLOR_FORCE' in default_env:
    del default_env['CLICOLOR_FORCE']
default_env['TERM'] = ''
RMPP_PATH = os.path.abspath('./build/rm++')

def run(flags, pipe=False, env=default_env):
    with tempfile.TemporaryDirectory() as d:
        os.chdir(d)
        with open('some_file', 'w') as f:
            f.write('')
            f.flush()
        rmpp_cmd = '{} {}'.format(RMPP_PATH, flags)
        try:
            if pipe:
                output = subprocess.check_output([rmpp_cmd], shell=True, env=env)
            elif platform.system() == 'Darwin':
                output = subprocess.check_output(['script', '-q', '/dev/null', 'bash', '-c', rmpp_cmd],
                                                 env=env)
            else:
                output = subprocess.check_output(['script', '-qfec', rmpp_cmd, '/dev/null'],
                                                 env=env)
        except subprocess.CalledProcessError as err:
            sys.stdout.buffer.write(err.output)
            raise err
        leftover_files = subprocess.check_output('ls').decode('utf-8')
    final_output = ''
    for line in output.decode('utf-8').splitlines(True):
        if len(line) > 0 and line[-1] == '\r':
            continue
        final_output += line.replace('\r', '')
    return (final_output, leftover_files)

class Output(unittest.TestCase):
    def test_version(self):
        self.assertEqual(run('--version'), ('rm++ 0.2\nCopyright © 2019 Jan Niklas Hasse\nLicense GPLv3+: GNU GPL version 3 or later <https://gnu.org/licenses/gpl.html>.\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\n', 'some_file\n'))

    def test_remove_file(self):
        self.assertEqual(run('-rf some_file'), ('', ''))
        self.assertEqual(run('-r some_file'), ('', ''))
        self.assertEqual(run('-f some_file'), ('', ''))
        self.assertEqual(run('some_file'), ('', ''))

if __name__ == '__main__':
    unittest.main()
