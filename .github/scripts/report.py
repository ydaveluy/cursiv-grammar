#!/usr/bin/env python3
"""Render the per-language CI overview; missing/failed configurations fail it."""
import argparse
import json
import os
from pathlib import Path
import sys

COMPILERS = ['GCC', 'Clang']


def load(files, language, compiler):
    """Return the valid result of one build, or None when it is missing."""
    matches = files.get(f'{language}-{compiler}.json', [])
    try:
        result = json.loads(matches[0].read_text()) if len(matches) == 1 else None
    except (OSError, ValueError):
        return None
    if (isinstance(result, dict) and result.get('language') == language
            and result.get('compiler') == compiler):
        return result
    return None


def failed_stage(result):
    """Return the stage that failed, or 'missing' without a valid result."""
    if result is None:
        return 'missing'
    for stage in result.get('stages', []):
        if stage.get('exit_code'):
            return stage.get('stage', 'unknown')
    return 'unknown'


def table(header, rows):
    return ['| ' + ' | '.join(header) + ' |', '|' + '---|' * len(header), *rows]


def report(languages, results):
    if not languages:
        return 'No language build required for these changes.\n', False
    files = {}
    for path in results.rglob('*.json'):
        files.setdefault(path.name, []).append(path)
    everything, failures = [], []
    for language in languages:
        cells, failed_cells = [], []
        for compiler in COMPILERS:
            result = load(files, language, compiler)
            if result is not None and result.get('status') == 'OK':
                cells.append('✅')
                failed_cells.append('✅')
            else:
                cells.append('❌')
                failed_cells.append('❌ ' + failed_stage(result))
        everything.append(f'| {language} | ' + ' | '.join(cells) + ' |')
        if any(cell != '✅' for cell in cells):
            failures.append(f'| {language} | ' + ' | '.join(failed_cells) + ' |')
    header = ['Language', *COMPILERS]
    count = len(languages)
    lines = [f"**{count} language{'s' if count != 1 else ''}: "
             f'{count - len(failures)} OK, {len(failures)} KO**', '']
    if failures:
        lines += ['### Failed languages', '', *table(header, failures), '']
    lines += ['<details>', f'<summary>All {count} languages</summary>', '',
              *table(header, everything), '', '</details>']
    return '\n'.join(lines) + '\n', bool(failures)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--results', type=Path, default=Path('build/ci-results'))
    parser.add_argument('--language')
    parser.add_argument('--languages', default=os.environ.get('CI_LANGUAGES', '[]'))
    args = parser.parse_args()
    languages = [args.language] if args.language else json.loads(args.languages)
    markdown, failed = report(languages, args.results)
    print(markdown)
    if os.environ.get('GITHUB_STEP_SUMMARY'):
        with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as stream:
            stream.write(markdown)
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
