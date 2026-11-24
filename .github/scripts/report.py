#!/usr/bin/env python3
"""Render one OK / KO table of the languages in CI_LANGUAGES; a missing or failed build fails it."""
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


def table(header, rows):
    return ['| ' + ' | '.join(header) + ' |', '|' + '---|' * len(header), *rows]


def report(languages, results):
    if not languages:
        return 'No language build required for these changes.\n', False
    files = {}
    for path in results.rglob('*.json'):
        files.setdefault(path.name, []).append(path)
    failed, passed = [], []
    for language in languages:
        cells = []
        for compiler in COMPILERS:
            result = load(files, language, compiler)
            cells.append('✅ OK' if result is not None and result.get('status') == 'OK' else '❌ KO')
        row = f'| {language} | ' + ' | '.join(cells) + ' |'
        (passed if all(cell == '✅ OK' for cell in cells) else failed).append(row)
    count = len(languages)
    lines = [f"**{count} language{'s' if count != 1 else ''}: "
             f'{len(passed)} OK, {len(failed)} KO**', '',
             *table(['Language', *COMPILERS], failed + passed)]
    return '\n'.join(lines) + '\n', bool(failed)


def main():
    languages = json.loads(os.environ.get('CI_LANGUAGES', '[]'))
    markdown, failed = report(languages, Path(__file__).resolve().parents[2] / 'build' / 'ci-results')
    print(markdown)
    if os.environ.get('GITHUB_STEP_SUMMARY'):
        with open(os.environ['GITHUB_STEP_SUMMARY'], 'a') as stream:
            stream.write(markdown)
    return int(failed)


if __name__ == '__main__':
    sys.exit(main())
