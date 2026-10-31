#!/usr/bin/env python3
"""Render the per-language CI overview; missing/failed configurations fail it."""
import argparse
import json
import os
from pathlib import Path
import sys

ENGINES = ['LL', 'LR']


def report(languages, results):
    rows = ['| Language | LL | LR | Result |',
            '|---|---|---|---|']
    failed = False
    for language in languages:
        statuses = []
        for engine in ENGINES:
            matches = list(results.rglob(f'{language}-{engine}.json'))
            try:
                result = json.loads(matches[0].read_text()) if len(matches) == 1 else {}
            except (OSError, ValueError):
                result = {}
            ok = (result.get('status') == 'OK' and result.get('language') == language
                  and result.get('engine') == engine)
            statuses.append('✅' if ok else '❌')
        ok = all(s == '✅' for s in statuses)
        failed |= not ok
        rows.append('| ' + ' | '.join([language, *statuses, '✅ OK' if ok else '❌ KO']) + ' |')
    if not languages:
        return 'No language build required for these changes.\n', False
    return '\n'.join(rows) + '\n', failed


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
