#!/usr/bin/env python3
"""Build and test one language against an installed Cursiv; record its verdict.

CC and CXX select the compilers.
"""
import argparse
import json
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


def run(root, language, compiler, cursiv):
    languages = (root / 'languages').resolve()
    source = (languages / language).resolve()
    if source.parent != languages or not (source / 'CMakeLists.txt').is_file():
        raise ValueError('Unknown language directory: ' + language)
    label = f'{language}-{compiler}'
    build = root / 'build' / 'ci' / label
    results = root / 'build' / 'ci-results'
    results.mkdir(parents=True, exist_ok=True)
    commands = [
        ('configure', ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                       '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_SCAN_FOR_MODULES=OFF',
                       '-DCMAKE_CXX_FLAGS=-Wall -Wextra -Werror',
                       f'-DCMAKE_PREFIX_PATH={cursiv.resolve()}',
                       # A package that is not found is an error, never a build of Cursiv.
                       '-DFETCHCONTENT_FULLY_DISCONNECTED=ON']),
        # Two units at a time: the generated parsers of a large grammar are heavy.
        ('build', ['cmake', '--build', str(build), '--parallel', '2']),
        ('test', ['ctest', '--test-dir', str(build), '--output-on-failure',
                  '--no-tests=error', '--timeout', '300',
                  '--output-junit', str(results / (label + '.xml'))])]
    result = {'language': language, 'compiler': compiler, 'status': 'KO', 'stages': []}
    for name, command in commands:
        result['stages'].append(stage(name, command, results / f'{label}-{name}.log'))
        if result['stages'][-1]['exit_code']:
            break
    else:
        result['status'] = 'OK'
    (results / (label + '.json')).write_text(json.dumps(result, indent=2) + '\n')
    return int(result['status'] != 'OK')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--language', required=True)
    parser.add_argument('--compiler', required=True, choices=COMPILERS)
    parser.add_argument('--cursiv', required=True, type=Path,
                        help='prefix of the installed Cursiv')
    args = parser.parse_args()
    try:
        return run(ROOT, args.language, args.compiler, args.cursiv)
    except ValueError as error:
        parser.error(str(error))


if __name__ == '__main__':
    sys.exit(main())
