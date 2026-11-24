#!/usr/bin/env python3
"""Build and test languages in one tree, so Cursiv compiles once; record one verdict per language.

CI_LANGUAGES holds the JSON list of languages; CC and CXX select the compilers.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import sys
import time

from report import COMPILERS

ROOT = Path(__file__).resolve().parents[2]


def execute(command, log):
    print('+ ' + ' '.join(map(str, command)), flush=True)
    with log.open('w', encoding='utf-8') as stream:
        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, errors='replace') as process:
            for line in process.stdout:
                print(line, end='', flush=True)
                stream.write(line)
        return process.returncode


def stage(name, command, log):
    started = time.monotonic()
    try:
        code = execute(command, log)
    except OSError as error:
        log.write_text(str(error) + '\n')
        code = 1
    return {'stage': name, 'exit_code': code,
            'seconds': round(time.monotonic() - started, 3), 'log': log.name}


def run(root, languages, compiler, cursiv_source):
    # An empty CURSIV_GRAMMARS would build every language.
    if not languages:
        raise ValueError('No language selected')
    build = root / 'build' / 'ci' / compiler
    results = root / 'build' / 'ci-results'
    results.mkdir(parents=True, exist_ok=True)
    jobs = str(os.cpu_count() or 1)
    configure = ['cmake', '-S', str(root), '-B', str(build), '-G', 'Ninja',
                 '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_FLAGS=-Wall -Wextra -Werror',
                 '-DCURSIV_GRAMMARS=' + ';'.join(languages),
                 f'-DCURSIV_SOURCE_DIR={cursiv_source.resolve()}']
    configured = stage('configure', configure, results / f'{compiler}-configure.log')
    if not configured['exit_code']:
        # Every language at once, so Ninja overlaps them; a failure stops no other language.
        stage('build', ['cmake', '--build', str(build), '--parallel', jobs, '--', '-k', '0'],
              results / f'{compiler}-build.log')
    failed = False
    for language in languages:
        label = f'{language}-{compiler}'
        result = {'language': language, 'compiler': compiler,
                  'status': 'KO', 'stages': [configured]}
        if not configured['exit_code']:
            # A no-op once built; otherwise it repeats this language's errors in its own log.
            commands = [('build', ['cmake', '--build', str(build), '--parallel', jobs,
                                   '--target', f'languages/{language}/all']),
                        ('test', ['ctest', '--test-dir', str(build / 'languages' / language),
                                  '--output-on-failure', '--no-tests=error', '--timeout', '300',
                                  '--parallel', jobs, '--output-junit', str(results / (label + '.xml'))])]
            for name, command in commands:
                result['stages'].append(stage(name, command, results / f'{label}-{name}.log'))
                if result['stages'][-1]['exit_code']:
                    break
            else:
                result['status'] = 'OK'
        failed = failed or result['status'] != 'OK'
        (results / (label + '.json')).write_text(json.dumps(result, indent=2) + '\n')
    return int(failed)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compiler', required=True, choices=COMPILERS)
    parser.add_argument('--cursiv-source', required=True, type=Path)
    args = parser.parse_args()
    try:
        return run(ROOT, json.loads(os.environ.get('CI_LANGUAGES', '[]')),
                   args.compiler, args.cursiv_source)
    except ValueError as error:
        parser.error(str(error))


if __name__ == '__main__':
    sys.exit(main())
