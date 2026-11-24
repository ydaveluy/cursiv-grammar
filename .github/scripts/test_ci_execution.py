"""Check CI failure propagation and visible per-language verdicts."""
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
        # CI_TEST_<STAGE> is the exit code of a stage.
        fake = """#!/usr/bin/env python3
import json, os, pathlib, sys
stage = 'test' if pathlib.Path(sys.argv[0]).name == 'ctest' else ('build' if '--build' in sys.argv else 'configure')
with open(os.environ['CI_TEST_COMMANDS'], 'a') as log:
    log.write(json.dumps({'stage': stage, 'args': sys.argv[1:]}) + '\\n')
print(stage + ' output')
sys.exit(int(os.environ.get('CI_TEST_' + stage.upper(), '0')))
"""
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
        self.results = self.root / 'build' / 'ci-results'

    def run_language(self):
        with contextlib.redirect_stdout(io.StringIO()):
            code = run_language.run(self.root, 'csv', 'GCC', self.root / 'cursiv')
        data = json.loads((self.results / 'csv-GCC.json').read_text())
        commands = [json.loads(s) for s in (self.root / 'commands').read_text().splitlines()]
        return code, data, commands

    def test_the_language_builds_against_the_installed_cursiv(self):
        code, data, commands = self.run_language()
        self.assertEqual(code, 0)
        self.assertEqual(data['status'], 'OK')
        self.assertEqual([c['stage'] for c in commands], ['configure', 'build', 'test'])
        configure = commands[0]['args']
        self.assertIn(str((self.root / 'languages' / 'csv').resolve()), configure)
        self.assertIn(f"-DCMAKE_PREFIX_PATH={(self.root / 'cursiv').resolve()}", configure)
        self.assertIn('-DFETCHCONTENT_FULLY_DISCONNECTED=ON', configure)
        self.assertIn('-DCMAKE_CXX_FLAGS=-Wall -Wextra -Werror', configure)
        self.assertIn('--no-tests=error', commands[2]['args'])
        # One table for the whole run comes from the report, none per build.
        self.assertFalse((self.root / 'summary').exists())

    def test_configure_failure_stops_the_language(self):
        with patch.dict(os.environ, {'CI_TEST_CONFIGURE': '2'}):
            code, data, commands = self.run_language()
        self.assertEqual(code, 1)
        self.assertEqual(data['status'], 'KO')
        self.assertEqual(len(commands), 1)
        self.assertTrue((self.results / 'csv-GCC-configure.log').exists())

    def test_build_failure_cannot_report_success(self):
        with patch.dict(os.environ, {'CI_TEST_BUILD': '2'}):
            code, data, commands = self.run_language()
        self.assertEqual(code, 1)
        self.assertEqual(data['status'], 'KO')
        self.assertEqual(data['stages'][-1]['stage'], 'build')
        self.assertEqual(len(commands), 2)

    def test_test_failure_cannot_report_success(self):
        with patch.dict(os.environ, {'CI_TEST_TEST': '8'}):
            code, data, commands = self.run_language()
        self.assertEqual(code, 1)
        self.assertEqual(data['status'], 'KO')
        self.assertEqual(data['stages'][-1]['exit_code'], 8)
        self.assertEqual(len(commands), 3)

    def test_a_parent_path_is_no_language(self):
        (self.root / 'CMakeLists.txt').write_text('project(all)\n')
        for language in ['..', 'csv/..', 'missing']:
            with self.assertRaises(ValueError):
                run_language.run(self.root, language, 'GCC', self.root / 'cursiv')
        self.assertFalse((self.root / 'commands').exists())


class ReportTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)

    def write(self, language, compiler, status='OK', failed=None):
        stages = [{'stage': 'configure', 'exit_code': 0}]
        if failed:
            stages.append({'stage': failed, 'exit_code': 2})
        (self.root / f'{language}-{compiler}.json').write_text(json.dumps({
            'status': status, 'language': language, 'compiler': compiler, 'stages': stages}))

    def fill(self, language):
        for compiler in report.COMPILERS:
            self.write(language, compiler)

    def test_overview_keeps_successful_and_failed_languages(self):
        self.fill('csv')
        self.fill('json')
        self.write('json', 'Clang', 'KO', 'build')
        text, failed = report.report(['csv', 'json'], self.root)
        self.assertTrue(failed)
        self.assertTrue(text.startswith('**2 languages: 1 OK, 1 KO**\n'))
        self.assertIn('| csv | ✅ OK | ✅ OK |', text)
        self.assertIn('| json | ✅ OK | ❌ KO |', text)

    def test_missing_or_truncated_results_are_not_success(self):
        self.fill('csv')
        (self.root / 'csv-GCC.json').unlink()
        (self.root / 'csv-Clang.json').write_text('{')
        text, failed = report.report(['csv'], self.root)
        self.assertTrue(failed)
        self.assertIn('| csv | ❌ KO | ❌ KO |', text)

    def test_result_of_another_configuration_is_not_success(self):
        self.fill('csv')
        self.write('csv', 'GCC')
        path = self.root / 'csv-GCC.json'
        path.write_text(path.read_text().replace('"csv"', '"json"'))
        _, failed = report.report(['csv'], self.root)
        self.assertTrue(failed)

    def test_one_table_with_failures_first(self):
        for language in ['csv', 'json', 'xml']:
            self.fill(language)
        self.write('json', 'GCC', 'KO', 'test')
        self.write('xml', 'Clang', 'KO', 'cleanup')
        (self.root / 'xml-GCC.json').unlink()
        text, failed = report.report(['csv', 'json', 'xml'], self.root)
        self.assertTrue(failed)
        self.assertEqual(text, '**3 languages: 1 OK, 2 KO**\n\n'
                               '| Language | GCC | Clang |\n|---|---|---|\n'
                               '| json | ❌ KO | ✅ OK |\n'
                               '| xml | ❌ KO | ❌ KO |\n'
                               '| csv | ✅ OK | ✅ OK |\n')

    def test_all_ok(self):
        self.fill('csv')
        text, failed = report.report(['csv'], self.root)
        self.assertFalse(failed)
        self.assertTrue(text.startswith('**1 language: 1 OK, 0 KO**\n'))
        self.assertIn('| csv | ✅ OK | ✅ OK |', text)

    def test_empty_selection_has_no_failure(self):
        text, failed = report.report([], self.root)
        self.assertFalse(failed)
        self.assertIn('No language build required', text)


if __name__ == '__main__':
    unittest.main()
