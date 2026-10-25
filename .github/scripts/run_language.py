#!/usr/bin/env python3
"""Build and test one standalone language/configuration; retain failure logs."""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time


def execute(command, log):
    print('+ ' + ' '.join(map(str, command)), flush=True)
    with log.open('w', encoding='utf-8') as stream:
        with subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, errors='replace') as process:
            for line in process.stdout:
                print(line, end='', flush=True)
                stream.write(line)
        return process.returncode


def run(args):
    source = args.repo_root.resolve() / 'languages' / args.language
    if args.language in {'.', '..'} or Path(args.language).name != args.language or not (source / 'CMakeLists.txt').is_file():
        raise ValueError('Unknown language directory: ' + args.language)
    if args.jobs < 1:
        raise ValueError('--jobs must be positive')
    label = f'{args.language}-{args.compiler_label}-{args.engine}'
    build = args.build_root.resolve() / label
    results = args.results_dir.resolve()
    results.mkdir(parents=True, exist_ok=True)
    result = {'language': args.language, 'compiler': args.compiler_label,
              'engine': args.engine, 'status': 'KO', 'stages': []}
    configure = ['cmake', '-S', str(source), '-B', str(build), '-G', 'Ninja',
                 '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_CXX_SCAN_FOR_MODULES=OFF',
                 '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON', '-DCURSIV_BUILD_TESTS=OFF',
                 '-DCMAKE_CXX_FLAGS=' + ('-Wall -Wextra -Werror ' + args.cxx_flags).strip(),
                 f'-DCURSIV_SOURCE_DIR={args.cursiv_source.resolve()}',
                 f'-DCMAKE_C_COMPILER={args.cc}', f'-DCMAKE_CXX_COMPILER={args.cxx}',
                 f'-DCURSIV_DEFAULT_ENGINE={args.engine}', *args.cmake_arg]
    commands = [('configure', configure),
                ('build', ['cmake', '--build', str(build), '--parallel', str(args.jobs)]),
                ('test', ['ctest', '--test-dir', str(build), '--output-on-failure',
                          '--no-tests=error', '--timeout', '300', '--output-junit', str(results / (label + '.xml'))])]
    try:
        for stage, command in commands:
            started = time.monotonic()
            log = results / f'{label}-{stage}.log'
            try:
                code = execute(command, log)
            except OSError as error:
                log.write_text(str(error) + '\n')
                code = 1
            result['stages'].append({'stage': stage, 'exit_code': code,
                                     'seconds': round(time.monotonic() - started, 3),
                                     'log': log.name})
            if code:
                break
        else:
            result['status'] = 'OK'
    finally:
        if args.clean_build and build.exists():
            try:
                shutil.rmtree(build)
            except OSError as error:
                result['status'] = 'KO'
                log = results / (label + '-cleanup.log')
                log.write_text(str(error) + '\n')
                result['stages'].append({'stage': 'cleanup', 'exit_code': 1, 'log': log.name})
        summary = (f"| {args.language} | {args.compiler_label} | {args.engine} | "
                   f"{'✅ OK' if result['status'] == 'OK' else '❌ KO'} |\n")
        if os.environ.get('GITHUB_STEP_SUMMARY'):
            with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as stream:
                stream.write('| Language | Compiler | Engine | Build + test |\n|---|---|---|---|\n' + summary)
        (results / (label + '.json')).write_text(json.dumps(result, indent=2) + '\n')
    return 0 if result['status'] == 'OK' else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--language', required=True)
    parser.add_argument('--compiler-label', required=True, choices=['GCC', 'Clang'])
    parser.add_argument('--cxx', required=True)
    parser.add_argument('--cc', required=True)
    parser.add_argument('--engine', required=True, choices=['LL', 'LR'])
    parser.add_argument('--cursiv-source', required=True, type=Path)
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--build-root', type=Path, default=Path('build/ci'))
    parser.add_argument('--results-dir', type=Path, default=Path('build/ci-results'))
    parser.add_argument('--jobs', type=int, default=2)
    parser.add_argument('--cxx-flags', default='', help='Additional C++ flags after -Wall -Wextra -Werror')
    parser.add_argument('--cmake-arg', action='append', default=[])
    parser.add_argument('--clean-build', action='store_true')
    args = parser.parse_args()
    try:
        return run(args)
    except ValueError as error:
        parser.error(str(error))


if __name__ == '__main__':
    sys.exit(main())
