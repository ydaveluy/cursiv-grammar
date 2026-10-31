"""Check CI failure propagation and visible per-language verdicts."""
import argparse
import contextlib
import io
import json
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import report
import run_language


class ExecutionTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        source = self.root / 'languages' / 'csv'
        source.mkdir(parents=True)
        (source / 'CMakeLists.txt').write_text('project(csv)\n')
        tools = self.root / 'bin'
        tools.mkdir()
        fake = '''#!/usr/bin/env python3
import json, os, pathlib, sys
stage = 'test' if pathlib.Path(sys.argv[0]).name == 'ctest' else ('build' if '--build' in sys.argv else 'configure')
with open(os.environ['CI_TEST_COMMANDS'], 'a') as log:
    log.write(json.dumps({'stage': stage, 'args': sys.argv[1:]}) + '\\n')
print(stage + ' output')
sys.exit(int(os.environ.get('CI_TEST_' + stage.upper(), '0')))
'''
        for name in ['cmake', 'ctest']:
            tool = tools / name
            tool.write_text(fake)
            tool.chmod(0o755)
        self.environment = patch.dict(os.environ, {
            'PATH': str(tools) + os.pathsep + os.environ['PATH'],
            'CI_TEST_COMMANDS': str(self.root / 'commands'),
            'GITHUB_STEP_SUMMARY': str(self.root / 'summary'),
        })
        self.environment.start()
        self.addCleanup(self.environment.stop)
        self.args = argparse.Namespace(repo_root=self.root, language='csv',
            engine='LL', cxx='g++-14', cc='gcc-14', jobs=2,
            build_root=self.root / 'build', results_dir=self.root / 'results',
            cursiv_source=self.root / 'engine', cmake_arg=[], cxx_flags='', clean_build=False)

    def run_variant(self):
        with contextlib.redirect_stdout(io.StringIO()):
            code = run_language.run(self.args)
        data = json.loads((self.args.results_dir / 'csv-LL.json').read_text())
        commands = [json.loads(s) for s in (self.root / 'commands').read_text().splitlines()]
        return code, data, commands

    def test_build_and_all_ctest_are_required(self):
        code, data, commands = self.run_variant()
        self.assertEqual(code, 0)
        self.assertEqual(data['status'], 'OK')
        self.assertEqual([c['stage'] for c in commands], ['configure', 'build', 'test'])
        self.assertIn('--no-tests=error', commands[-1]['args'])
        self.assertIn('-DCMAKE_C_COMPILER=gcc-14', commands[0]['args'])
        self.assertIn('-DCMAKE_CXX_COMPILER=g++-14', commands[0]['args'])
        self.assertIn('-DCMAKE_CXX_FLAGS=-Wall -Wextra -Werror', commands[0]['args'])
        self.assertIn('✅ OK', (self.root / 'summary').read_text())

    def test_configure_failure_stops_this_variant(self):
        with patch.dict(os.environ, {'CI_TEST_CONFIGURE': '2'}):
            code, data, commands = self.run_variant()
        self.assertEqual(code, 1)
        self.assertEqual(data['status'], 'KO')
        self.assertEqual(len(commands), 1)
        self.assertTrue((self.args.results_dir / 'csv-LL-configure.log').exists())

    def test_build_failure_cannot_report_success(self):
        with patch.dict(os.environ, {'CI_TEST_BUILD': '2'}):
            code, data, commands = self.run_variant()
        self.assertEqual(code, 1)
        self.assertEqual(data['stages'][-1]['stage'], 'build')
        self.assertEqual(len(commands), 2)
        self.assertIn('❌ KO', (self.root / 'summary').read_text())

    def test_test_failure_cannot_report_success(self):
        with patch.dict(os.environ, {'CI_TEST_TEST': '8'}):
            code, data, commands = self.run_variant()
        self.assertEqual(code, 1)
        self.assertEqual(data['status'], 'KO')
        self.assertEqual(data['stages'][-1]['exit_code'], 8)
        self.assertEqual(len(commands), 3)

    def test_cleanup_failure_is_not_a_green_result(self):
        self.args.clean_build = True
        (self.args.build_root / 'csv-LL').mkdir(parents=True)
        with patch.object(run_language.shutil, 'rmtree', side_effect=OSError('cleanup failed')):
            code, data, _ = self.run_variant()
        self.assertEqual(code, 1)
        self.assertEqual(data['status'], 'KO')
        self.assertEqual(data['stages'][-1]['stage'], 'cleanup')

    def test_parent_path_cannot_build_all_languages(self):
        self.args.language = '..'
        with self.assertRaises(ValueError):
            run_language.run(self.args)
        self.assertFalse((self.root / 'commands').exists())


class ReportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def fill(self, language):
        for engine in report.ENGINES:
            (self.root / f'{language}-{engine}.json').write_text(json.dumps({
                'status': 'OK', 'language': language, 'engine': engine}))

    def test_overview_keeps_successful_and_failed_languages(self):
        self.fill('csv')
        self.fill('json')
        (self.root / 'json-LR.json').write_text('{"status": "KO"}')
        text, failed = report.report(['csv', 'json'], self.root)
        self.assertTrue(failed)
        self.assertIn('| csv | ✅ | ✅ | ✅ OK |', text)
        self.assertIn('| json | ✅ | ❌ | ❌ KO |', text)

    def test_missing_or_truncated_results_are_not_success(self):
        self.fill('csv')
        (self.root / 'csv-LL.json').unlink()
        (self.root / 'csv-LR.json').write_text('{')
        text, failed = report.report(['csv'], self.root)
        self.assertTrue(failed)
        self.assertIn('| csv | ❌ | ❌ | ❌ KO |', text)

    def test_empty_selection_has_no_failure(self):
        text, failed = report.report([], self.root)
        self.assertFalse(failed)
        self.assertIn('No language build required', text)


if __name__ == '__main__':
    unittest.main()
