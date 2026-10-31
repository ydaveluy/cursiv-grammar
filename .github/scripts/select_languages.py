#!/usr/bin/env python3
"""Select independent language CI jobs from the checked-out language inventory.

The checkout supplies languages/* containing both CMakeLists.txt and src/.
Pushes compare base..head. Missing history selects every language, as do a
Cursiv dispatch and --force-all.

Root README files and doc/, docs/, audit/ are documentation-only. Changes in
bench/, .github/ or root CMake configuration select everything. Any other
unknown path also selects everything conservatively: a new build dependency
must not silently escape CI. Changes within a language select that existing
language, including its documentation. Deleted/incomplete ports are filtered.

Empty selections have two empty matrix groups, with no placeholder build.
The workflow must skip its build jobs when their group is empty.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
import sys


MAX_GROUP_SIZE = 256
EVENTS = ("push", "repository_dispatch")


class SelectionError(ValueError):
    """A selection cannot be represented safely as a CI matrix."""


def inventory(repo: Path) -> list[str]:
    languages = repo / "languages"
    if not languages.is_dir():
        return []
    return sorted(
        port.name
        for port in languages.iterdir()
        if port.is_dir()
        and (port / "CMakeLists.txt").is_file()
        and (port / "src").is_dir()
    )


def git(repo: Path, *args: str) -> bytes:
    return subprocess.run(
        ["git", "-C", str(repo), *args],
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ).stdout


def resolve_commit(repo: Path, ref: str) -> str | None:
    if not ref or not ref.strip("0"):
        return None
    try:
        return git(
            repo, "rev-parse", "--verify", "--end-of-options", f"{ref}^{{commit}}"
        ).decode("ascii").strip()
    except (subprocess.CalledProcessError, OSError):
        return None


def result(languages: list[str], reason: str) -> dict:
    if len(languages) > 2 * MAX_GROUP_SIZE:
        raise SelectionError(
            f"{len(languages)} languages selected; the two CI groups support "
            f"at most {2 * MAX_GROUP_SIZE} languages "
            f"({MAX_GROUP_SIZE} per group)"
        )
    middle = (len(languages) + 1) // 2
    return {
        "languages": languages,
        "group_1": languages[:middle],
        "group_2": languages[middle:],
        "count": len(languages),
        "reason": reason,
    }


def is_documentation(path: str) -> bool:
    if path.startswith(("doc/", "docs/", "audit/")):
        return True
    name = path.lower()
    return "/" not in path and (name == "readme" or name.startswith("readme."))


def select_languages(
    repo: Path,
    *,
    base: str,
    head: str,
    event: str,
    force_all: bool = False,
) -> dict:
    if event not in EVENTS:
        raise SelectionError(f"unsupported event: {event}")
    available = inventory(repo)
    if force_all:
        return result(available, "force-all")
    if event == "repository_dispatch":
        return result(available, "cursiv-dispatch")

    base_commit = resolve_commit(repo, base)
    if base_commit is None:
        return result(available, "base-unavailable")
    head_commit = resolve_commit(repo, head)
    if head_commit is None:
        return result(available, "head-unavailable")

    try:
        # Disable rename detection so both the removed and added path are
        # classified, even when an unchanged file moves between two ports.
        diff = git(
            repo, "diff", "--name-only", "-z", "--no-renames",
            base_commit, head_commit, "--",
        )
    except (subprocess.CalledProcessError, OSError):
        return result(available, "diff-unavailable")

    paths = [os.fsdecode(path) for path in diff.split(b"\0") if path]
    if not paths:
        return result([], "no-changes")
    existing = set(available)
    selected = set()
    language_change = False
    for path in paths:
        if path in ("CMakeLists.txt", "CMakePresets.json") or path.startswith(
            ("bench/", ".github/")
        ):
            return result(available, "common-files-changed")
        parts = path.split("/")
        if len(parts) >= 3 and parts[0] == "languages":
            language_change = True
            if parts[1] in existing:
                selected.add(parts[1])
        elif not is_documentation(path):
            return result(available, "unknown-files-changed")
    if selected:
        return result(sorted(selected), "language-files-changed")
    return result(
        [], "no-existing-languages-changed" if language_change else "documentation-only"
    )


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="", help="push before SHA")
    parser.add_argument("--head", default="HEAD", help="push head SHA")
    parser.add_argument("--event", required=True, choices=EVENTS)
    parser.add_argument("--force-all", action="store_true")
    parser.add_argument("--output", type=Path, help="also write the selection as JSON")
    args = parser.parse_args(argv)
    try:
        selection = select_languages(
            Path(__file__).resolve().parents[2],
            base=args.base,
            head=args.head,
            event=args.event,
            force_all=args.force_all,
        )
        encoded = json.dumps(selection, ensure_ascii=True)
        if args.output:
            args.output.parent.mkdir(parents=True, exist_ok=True)
            args.output.write_text(encoded + "\n", encoding="utf-8")
        if output := os.environ.get("GITHUB_OUTPUT"):
            with open(output, "a", encoding="utf-8") as stream:
                for key, value in selection.items():
                    if isinstance(value, list):
                        value = json.dumps(value, ensure_ascii=True, separators=(",", ":"))
                    stream.write(f"{key}={value}\n")
    except (SelectionError, OSError) as error:
        parser.exit(2, f"select_languages: {error}\n")
    print(encoded)
    return 0


if __name__ == "__main__":
    sys.exit(main())
