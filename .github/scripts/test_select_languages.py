#!/usr/bin/env python3
"""Exercise the real Git diff and CLI paths without building a language."""

import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

import select_languages as selector


class SelectLanguagesTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.repo = Path(self.temporary.name)
        self.env = os.environ.copy()
        for name in ("GIT_DIR", "GIT_WORK_TREE", "GIT_INDEX_FILE"):
            self.env.pop(name, None)
        self.env.update(GIT_CONFIG_NOSYSTEM="1", GIT_CONFIG_GLOBAL=os.devnull)
        self.git("init", "-q", "-b", "main")
        self.git("config", "user.name", "Selector test")
        self.git("config", "user.email", "selector-test@example.invalid")
        self.git("config", "core.hooksPath", os.devnull)
        self.write("README.md", "Language repository\n")
        self.port("alpha")
        self.port("beta")
        self.base = self.commit()

    def git(self, *args):
        return subprocess.check_output(
            ["git", "-C", str(self.repo), *args], env=self.env, stderr=subprocess.PIPE
        ).decode().strip()

    def write(self, path, content="changed\n"):
        target = self.repo / path
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_text(content, encoding="utf-8")

    def port(self, name):
        self.write(f"languages/{name}/CMakeLists.txt", "project(port)\n")
        self.write(f"languages/{name}/src/source.cpp", "// source\n")
        self.write(f"languages/{name}/src/keep.hpp", "// keeps src in a fresh checkout\n")

    def commit(self):
        self.git("add", "-A")
        self.git("commit", "-q", "-m", "test revision")
        return self.git("rev-parse", "HEAD")

    def select(self, *, base=None, head="HEAD", event="push", force_all=False):
        return selector.select_languages(
            self.repo,
            base=self.base if base is None else base,
            head=head,
            event=event,
            force_all=force_all,
        )

    def test_push_selects_only_changed_language(self):
        self.write("languages/alpha/src/source.cpp")
        head = self.commit()
        selected = self.select(head=head)
        self.assertEqual(selected["languages"], ["alpha"])
        self.assertEqual(selected["group_1"], ["alpha"])
        self.assertEqual(selected["group_2"], [])
        self.assertEqual(selected["count"], 1)

    def test_push_compares_both_endpoints(self):
        self.git("checkout", "-q", "-b", "feature")
        self.write("languages/alpha/src/source.cpp", "// feature\n")
        head = self.commit()
        self.git("checkout", "-q", "main")
        self.write("languages/beta/src/source.cpp", "// target branch advanced\n")
        base = self.commit()
        self.git("checkout", "-q", "feature")
        self.assertEqual(
            self.select(base=base, head=head, event="push")["languages"],
            ["alpha", "beta"],
        )

    def test_nul_delimited_diff_preserves_spaces_and_newlines(self):
        self.port("with space")
        self.base = self.commit()
        self.write("languages/with space/src/a file\nwith a newline.cpp")
        self.commit()
        self.assertEqual(self.select()["languages"], ["with space"])

    def test_new_port_is_selected(self):
        self.port("new-port")
        self.commit()
        self.assertEqual(self.select()["languages"], ["new-port"])

    def test_rename_between_existing_ports_selects_both_sides(self):
        self.git("config", "diff.renames", "true")
        (self.repo / "languages/alpha/src/source.cpp").rename(
            self.repo / "languages/beta/src/renamed.cpp"
        )
        self.commit()
        self.assertEqual(self.select()["languages"], ["alpha", "beta"])

    def test_entire_port_rename_selects_only_the_existing_name(self):
        (self.repo / "languages/alpha").rename(self.repo / "languages/renamed")
        self.commit()
        self.assertEqual(self.select()["languages"], ["renamed"])

    def test_deleted_port_does_not_create_a_build_job(self):
        shutil.rmtree(self.repo / "languages/alpha")
        self.commit()
        selected = self.select()
        self.assertEqual(selected["languages"], [])
        self.assertEqual(selected["group_1"], [])
        self.assertEqual(selected["group_2"], [])
        self.assertEqual(selected["count"], 0)
        self.assertEqual(selected["reason"], "no-existing-languages-changed")

    def test_missing_or_null_base_selects_every_language(self):
        for base in ("", "0" * 40, "0" * 64, "missing-commit"):
            with self.subTest(base=base):
                selected = self.select(base=base)
                self.assertEqual(selected["languages"], ["alpha", "beta"])
                self.assertEqual(selected["reason"], "base-unavailable")

    def test_missing_head_selects_every_language(self):
        selected = self.select(head="missing-commit")
        self.assertEqual(selected["languages"], ["alpha", "beta"])
        self.assertEqual(selected["reason"], "head-unavailable")

    def test_common_paths_select_every_language(self):
        for path in (
            "CMakeLists.txt",
            "CMakePresets.json",
            "bench/driver.cpp",
            ".github/workflows/languages.yml",
        ):
            with self.subTest(path=path):
                self.write(path)
                head = self.commit()
                selected = self.select(head=head)
                self.assertEqual(selected["languages"], ["alpha", "beta"])
                self.assertEqual(selected["reason"], "common-files-changed")
                self.base = head

    def test_root_documentation_does_not_create_build_jobs(self):
        for path in ("README.md", "README.fr.md", "docs/guide.md", "audit/result.json"):
            self.write(path)
        self.commit()
        selected = self.select()
        self.assertEqual(selected["count"], 0)
        self.assertEqual(selected["languages"], [])
        self.assertEqual(selected["reason"], "documentation-only")

    def test_documentation_inside_a_port_still_selects_that_port(self):
        self.write("languages/alpha/README.md")
        self.commit()
        self.assertEqual(self.select()["languages"], ["alpha"])

    def test_unknown_build_paths_conservatively_select_every_language(self):
        self.write("cmake/new_dependency.cmake")
        self.commit()
        selected = self.select()
        self.assertEqual(selected["languages"], ["alpha", "beta"])
        self.assertEqual(selected["reason"], "unknown-files-changed")

    def test_empty_diff_ignores_uncommitted_files_and_has_empty_groups(self):
        self.write("languages/alpha/src/source.cpp", "// not in the event commits\n")
        selected = self.select(head=self.base)
        self.assertEqual(selected["count"], 0)
        self.assertEqual(selected["group_1"], [])
        self.assertEqual(selected["group_2"], [])
        self.assertEqual(selected["reason"], "no-changes")

    def test_incomplete_ports_are_not_in_the_inventory(self):
        self.write("languages/no-src/CMakeLists.txt")
        self.write("languages/no-cmake/src/source.cpp")
        self.commit()
        self.assertEqual(self.select()["languages"], [])
        self.assertEqual(selector.inventory(self.repo), ["alpha", "beta"])

    def test_cursiv_dispatch_and_forced_runs_select_every_language(self):
        self.assertEqual(
            self.select(event="repository_dispatch")["languages"], ["alpha", "beta"]
        )
        self.assertEqual(self.select(force_all=True)["reason"], "force-all")

    def test_316_languages_split_into_two_balanced_groups(self):
        for number in range(314):
            self.port(f"port-{number:03d}")
        selected = self.select(force_all=True)
        self.assertEqual(selected["count"], 316)
        self.assertEqual(len(selected["group_1"]), 158)
        self.assertEqual(len(selected["group_2"]), 158)
        self.assertEqual(
            selected["group_1"] + selected["group_2"], selected["languages"]
        )
        self.assertEqual(len(set(selected["languages"])), 316)

    def test_matrix_limit_is_explicit_and_512_is_supported(self):
        for number in range(510):
            self.port(f"port-{number:03d}")
        selected = self.select(force_all=True)
        self.assertEqual(len(selected["group_1"]), 256)
        self.assertEqual(len(selected["group_2"]), 256)
        self.port("one-too-many")
        with self.assertRaisesRegex(selector.SelectionError, "513.*at most 512"):
            self.select(force_all=True)

    def test_empty_inventory_is_supported(self):
        shutil.rmtree(self.repo / "languages")
        selected = self.select(force_all=True)
        self.assertEqual(selected["languages"], [])
        self.assertEqual(selected["count"], 0)

    def test_cli_writes_matching_json_and_github_outputs(self):
        self.write("languages/alpha/src/source.cpp")
        head = self.commit()
        script = self.repo / ".github/scripts/select_languages.py"
        script.parent.mkdir(parents=True)
        shutil.copyfile(Path(selector.__file__), script)
        output = self.repo / "results/selection.json"
        github_output = self.repo / "github-output"
        github_output.write_text("existing=preserved\n", encoding="utf-8")
        env = dict(self.env, GITHUB_OUTPUT=str(github_output))
        completed = subprocess.run(
            [
                sys.executable, str(script), "--base", self.base, "--head", head,
                "--event", "push", "--output", str(output),
            ],
            cwd=self.repo.parent,
            env=env,
            check=True,
            capture_output=True,
            text=True,
        )
        selected = json.loads(completed.stdout)
        self.assertEqual(selected, json.loads(output.read_text(encoding="utf-8")))
        self.assertEqual(selected["languages"], ["alpha"])
        outputs = dict(
            line.split("=", 1)
            for line in github_output.read_text(encoding="utf-8").splitlines()
        )
        self.assertEqual(outputs["existing"], "preserved")
        self.assertEqual(json.loads(outputs["languages"]), ["alpha"])
        self.assertEqual(json.loads(outputs["group_1"]), ["alpha"])
        self.assertEqual(json.loads(outputs["group_2"]), [])
        self.assertEqual(outputs["count"], "1")
        self.assertEqual(outputs["reason"], selected["reason"])


if __name__ == "__main__":
    unittest.main()
