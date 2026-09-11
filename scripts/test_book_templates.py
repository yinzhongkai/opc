"""Exercise optional book-template adoption using public, disposable fixtures.

These checks validate configuration and file links, not book quality or learning.
Run with the existing unittest discovery command.
"""

from __future__ import annotations

import copy
from pathlib import Path
import re
import shutil
import sys
import tempfile
import unittest

sys.dont_write_bytecode = True
from validate_framework import Validator, yaml


class BookTemplateIntegrationTests(unittest.TestCase):
    def setUp(self):
        source = Path(__file__).resolve().parents[1]
        temporary = tempfile.TemporaryDirectory(prefix="book-template-integration-")
        self.addCleanup(temporary.cleanup)
        self.root = Path(temporary.name)
        # Never copy actual projects or modify the shared working tree.
        for path in source.glob("*.md"):
            shutil.copy2(path, self.root / path.name)
        for name in ("roles", "knowledge", "adapters", "templates"):
            shutil.copytree(source / name, self.root / name)
        self.pack = self.root / "templates/book"
        guide = (self.pack / "README.md").read_text(encoding="utf-8")
        self.profiles = [
            yaml.safe_load(block)["roleKnowledge"]
            for block in re.findall(r"^```yaml\n(.*?)^```", guide, flags=re.M | re.S)
        ]
        self.assertEqual(2, len(self.profiles), "The guide has common and additive technical profiles")

    def create_project(self, name="sample-book"):
        project = self.root / "projects" / name
        shutil.copytree(self.root / "templates/project", project)
        for path in project.rglob("*"):
            if path.is_file():
                content = path.read_text(encoding="utf-8-sig")
                path.write_text(
                    content.replace("{{project_id}}", name).replace("{{project_name}}", name),
                    encoding="utf-8",
                )
        return project

    def read_team(self, project):
        return yaml.safe_load((project / "TEAM.yaml").read_text(encoding="utf-8"))

    def save_team(self, project, team):
        (project / "TEAM.yaml").write_text(
            yaml.safe_dump(team, sort_keys=False, allow_unicode=True), encoding="utf-8",
        )

    def apply_profiles(self, project, technical=False):
        team = self.read_team(project)
        for profile in self.profiles[:2 if technical else 1]:
            for role, additions in profile.items():
                current = team.setdefault("roleKnowledge", {}).setdefault(role, [])
                current.extend(item for item in additions if item not in current)
        self.save_team(project, team)
        return team

    def adopt(self, project, technical=False, chapters=("ch-introduction",)):
        book = project / "artifacts/book"
        book.mkdir()
        for name in ("design.md", "outline.md", "conventions.md", "glossary.md", "sources.md"):
            shutil.copy2(self.pack / name, book / name)
        if technical:
            shutil.copy2(self.pack / "environment.md", book / "environment.md")
            shutil.copytree(self.pack / "examples", book / "examples")
        for name in ("evidence", "releases"):
            shutil.copytree(self.pack / name, book / name)
        for chapter in chapters:
            shutil.copytree(self.pack / "chapters/ch-001", book / "chapters" / chapter)
        # The repository-relative setup guide is deliberately not a book artifact.
        self.assertFalse((book / "README.md").exists())
        return book

    def assert_valid(self):
        validator = Validator(self.root)
        self.assertEqual([], validator.run())
        return validator

    def test_pack_is_valid_without_real_projects_or_members(self):
        validator = self.assert_valid()
        self.assertEqual(0, validator.project_count)
        self.assertFalse(list(self.pack.rglob("*.yaml")))
        self.assertFalse(list(self.pack.rglob("TEAM.*")))
        self.assertEqual(
            {"plan.md", "text.md", "record.md"},
            {path.name for path in (self.pack / "chapters/ch-001").iterdir()},
        )

    def test_generic_project_remains_empty_and_does_not_adopt_book_methods(self):
        project = self.create_project("ordinary-project")
        team = self.read_team(project)
        self.assertEqual(3, team["schemaVersion"])
        self.assertEqual([], team["members"])
        self.assertEqual({}, team.get("roleKnowledge", {}))
        self.assertFalse((project / "artifacts/book").exists())
        self.assertFalse(list((project / "members").glob("*.yaml")))
        self.assert_valid()

    def test_documented_profiles_merge_without_replacing_existing_knowledge(self):
        project = self.create_project()
        team = self.read_team(project)
        team["roleKnowledge"] = {"writer": ["writing"]}
        self.save_team(project, team)
        self.apply_profiles(project)
        self.assert_valid()
        team = self.apply_profiles(project, technical=True)
        self.assertEqual("writing", team["roleKnowledge"]["writer"][0])
        self.assertIn("technical-book-validation", team["roleKnowledge"]["writer"])
        self.assertEqual([], team["members"])
        unchanged = copy.deepcopy(team)
        self.assertEqual(unchanged, self.apply_profiles(project, technical=True))
        self.assert_valid()

    def test_nontechnical_book_does_not_require_environment_or_experiments(self):
        project = self.create_project()
        team = self.apply_profiles(project)
        book = self.adopt(project)
        self.assertFalse((book / "environment.md").exists())
        self.assertFalse((book / "examples").exists())
        for additions in team["roleKnowledge"].values():
            self.assertNotIn("technical-book-validation", additions)
        self.assert_valid()

    def test_technical_book_copies_links_at_real_depth_with_stable_chapter_ids(self):
        project = self.create_project()
        self.apply_profiles(project, technical=True)
        book = self.adopt(project, technical=True, chapters=("ch-start", "ch-followup"))
        self.assertTrue((book / "environment.md").is_file())
        self.assertTrue((book / "examples/README.md").is_file())
        for chapter in ("ch-start", "ch-followup"):
            record = (book / "chapters" / chapter / "record.md").read_text(encoding="utf-8")
            self.assertIn("[正文](text.md)", record)
            self.assertIn("[蓝图](plan.md)", record)
        self.assertEqual(1, self.assert_valid().project_count)

    def test_profiles_allow_later_registration_without_numbered_member_ids(self):
        project = self.create_project()
        team = self.apply_profiles(project, technical=True)
        for role in ("planner", "writer", "reviewer", "developer", "project-manager"):
            team["members"].append(role)
            member = {"id": role, "role": role, "scope": ["执行已授权的本岗位图书工作"]}
            (project / "members" / f"{role}.yaml").write_text(
                yaml.safe_dump(member, sort_keys=False, allow_unicode=True), encoding="utf-8",
            )
            self.save_team(project, team)
            self.assert_valid()

    def test_broken_link_in_adopted_chapter_is_detected(self):
        project = self.create_project()
        book = self.adopt(project)
        (book / "chapters/ch-introduction/plan.md").unlink()
        errors = Validator(self.root).run()
        self.assertTrue(
            any("chapters/ch-introduction/record.md" in error
                and "本地链接目标不存在：plan.md" in error for error in errors), errors,
        )

    def test_missing_profile_knowledge_is_detected_before_member_creation(self):
        project = self.create_project()
        self.apply_profiles(project)
        (self.root / "knowledge/book-planning.md").unlink()
        errors = Validator(self.root).run()
        self.assertTrue(
            any("roleKnowledge['planner'] 引用了不存在的知识：book-planning" in error
                for error in errors), errors,
        )
        self.assertEqual([], self.read_team(project)["members"])


if __name__ == "__main__":
    unittest.main()
