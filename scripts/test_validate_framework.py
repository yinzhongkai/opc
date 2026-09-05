"""Targeted regression checks; fixtures live only in temporary directories.

Run: python -B -m unittest discover -s scripts -p "test_*.py" -v
"""

from __future__ import annotations

from contextlib import redirect_stdout
import copy
import io
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.dont_write_bytecode = True
from validate_framework import PROJECT_FILES, ROOT_FILES, Validator, main, yaml


class FrameworkValidationTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory(prefix="framework-validation-")
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        for name in ROOT_FILES:
            self.write(name, "# Framework\n")
        self.definition("knowledge", "team-management")
        self.definition("knowledge", "software-engineering")
        self.definition("roles", "project-manager", knowledge=["team-management"])
        self.definition("roles", "developer", knowledge=["software-engineering"])
        for name in PROJECT_FILES:
            self.write(f"templates/project/{name}", "# Template\n")
        self.team = {
            "schemaVersion": 3,
            "project": "{{project_id}}",
            "members": [],
        }
        self.save_team(self.team, template=True)

    def write(self, name, content):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        return path

    def definition(self, directory, identifier, status="active", knowledge=None):
        data = {"id": identifier, "name": identifier, "status": status}
        if knowledge is not None:
            data["knowledge"] = knowledge
        return self.write(
            f"{directory}/{identifier}.md",
            "---\n" + yaml.safe_dump(data, sort_keys=False) + "---\n\n# Definition\n",
        )

    def project(self, name="example"):
        folder = self.root / "projects" / name
        shutil.copytree(self.root / "templates/project", folder)
        data = copy.deepcopy(self.team)
        data["project"] = name
        self.save_team(data, project=name)
        return data

    def save_team(self, data, template=False, project="example"):
        folder = "templates/project" if template else f"projects/{project}"
        return self.write(
            f"{folder}/TEAM.yaml", yaml.safe_dump(data, sort_keys=False, allow_unicode=True),
        )

    def save_member(self, member, filename=None, project="example"):
        identifier = member["id"] if filename is None else filename
        return self.write(
            f"projects/{project}/members/{identifier}.yaml",
            yaml.safe_dump(member, sort_keys=False, allow_unicode=True),
        )

    def add_member(self, team, identifier="developer-01", role="developer"):
        member = {"id": identifier, "role": role, "scope": ["执行指定工作"]}
        team["members"].append(identifier)
        self.save_team(team)
        self.save_member(member)
        return member

    def errors(self):
        return Validator(self.root).run()

    def assert_valid(self):
        self.assertEqual([], self.errors())

    def assert_error(self, expected):
        errors = self.errors()
        self.assertTrue(any(expected in error for error in errors), errors)

    def test_no_projects_and_empty_projects_are_valid(self):
        self.assert_valid()
        (self.root / "projects").mkdir()
        self.assert_valid()
        (self.root / "projects/.reserved").mkdir()
        self.assert_valid()

    def test_empty_team_is_valid_in_template_and_real_project(self):
        self.assertEqual([], self.team["members"])
        data = self.project()
        self.assertEqual([], data["members"])
        self.assert_valid()

    def test_template_rejects_registered_members_even_with_valid_configuration(self):
        data = copy.deepcopy(self.team)
        data["members"] = ["developer-01"]
        self.save_team(data, template=True)
        self.write(
            "templates/project/members/developer-01.yaml",
            yaml.safe_dump({"id": "developer-01", "role": "developer", "scope": ["开发"]}),
        )
        self.assert_error("空白模板的 members 必须为空列表")

    def test_template_project_id_must_remain_a_placeholder(self):
        for project in ("hardcoded-project", "{{project_name}}", None):
            with self.subTest(project=project):
                data = copy.deepcopy(self.team)
                data["project"] = project
                self.save_team(data, template=True)
                self.assert_error("空白模板的 project 必须保留 {{project_id}} 占位符")
        self.save_team(self.team, template=True)
        self.assert_valid()

    def test_template_cannot_contain_visible_member_configuration_files(self):
        for name in ("example.yaml", "nested/example.yaml", "example.yml", "example.YAML"):
            with self.subTest(name=name):
                path = self.write(f"templates/project/members/{name}", "id: example\n")
                try:
                    self.assert_error(f"members/{name}: 空白模板不能预置成员配置文件")
                finally:
                    path.unlink()
        self.assert_valid()

    def test_real_project_records_reject_unreplaced_placeholders(self):
        self.project()
        for name in PROJECT_FILES:
            path = self.root / "projects/example" / name
            original = path.read_text(encoding="utf-8")
            for placeholder in ("{{project_id}}", "{{project_name}}"):
                with self.subTest(file=name, placeholder=placeholder):
                    # A comment is valid YAML and Markdown; only the placeholder
                    # should fail, even outside the TEAM.project field.
                    content = original + f"\n# {placeholder}\n"
                    path.write_text(content, encoding="utf-8")
                    try:
                        line = len(original.splitlines()) + 2
                        self.assert_error(
                            f"projects/example/{name}: 第 {line} 行存在未替换的项目占位符：{placeholder}"
                        )
                    finally:
                        path.write_text(original, encoding="utf-8")
        self.assert_valid()

    def test_real_project_placeholders_are_checked_inside_code_examples(self):
        self.project()
        self.write("projects/example/AGENTS.md", "# Project\n`{{project_id}}`\n```text\n{{project_name}}\n```\n")
        self.assert_error("AGENTS.md: 第 2 行存在未替换的项目占位符：{{project_id}}")
        self.assert_error("AGENTS.md: 第 4 行存在未替换的项目占位符：{{project_name}}")

    def test_business_templates_are_not_treated_as_framework_project_records(self):
        self.project()
        self.write("projects/example/src/page.html", "<h1>{{project_name}}</h1>\n")
        self.write("projects/example/artifacts/template-notes.md", "Example: `{{project_id}}`\n")
        self.write("projects/example/PROJECT.md", "# Project\nBusiness expression: {{page_title}}\n")
        self.assert_valid()

    def test_definition_extensions_must_be_lowercase_on_every_platform(self):
        for directory in ("roles", "knowledge"):
            for suffix in (".MD", ".Md", ".mD"):
                with self.subTest(directory=directory, suffix=suffix):
                    data = {"id": "case-probe", "name": "Case probe", "status": "active"}
                    if directory == "roles":
                        data["knowledge"] = ["software-engineering"]
                    path = self.write(
                        f"{directory}/case-probe{suffix}",
                        "---\n" + yaml.safe_dump(data) + "---\n# Definition\n",
                    )
                    try:
                        self.assert_error(f"{directory}/case-probe{suffix}: 定义文件扩展名必须为小写 .md")
                    finally:
                        path.unlink()
        self.assert_valid()

    def test_template_placeholder_is_rejected_in_real_project(self):
        self.assert_valid()
        data = self.project()
        data["project"] = "{{project_id}}"
        self.save_team(data)
        self.assert_error("真实项目不能使用占位符")

    def test_project_id_must_match_directory(self):
        data = self.project()
        data["project"] = "another-project"
        self.save_team(data)
        self.assert_error("project 必须与项目目录名一致")

    def test_reject_duplicate_yaml_keys_at_both_depths_and_in_frontmatter(self):
        data = self.project()
        self.add_member(data)
        path = self.root / "projects/example/TEAM.yaml"
        original = path.read_text(encoding="utf-8")
        for duplicate in (
            original + "schemaVersion: 3\n",
            original + "roleKnowledge:\n  developer: []\n  developer: []\n",
        ):
            with self.subTest(yaml=duplicate):
                path.write_text(duplicate, encoding="utf-8")
                self.assert_error("重复 YAML 键")
        path.write_text(original, encoding="utf-8")
        member_path = self.root / "projects/example/members/developer-01.yaml"
        original_member = member_path.read_text(encoding="utf-8")
        member_path.write_text(original_member + "role: developer\n", encoding="utf-8")
        self.assert_error("重复 YAML 键")
        member_path.write_text(original_member, encoding="utf-8")
        self.write(
            "knowledge/team-management.md",
            "---\nid: team-management\nname: test\nstatus: active\nstatus: draft\n---\n",
        )
        self.assert_error("重复 YAML 键")

    def test_reject_duplicate_member_id(self):
        data = self.project()
        self.add_member(data)
        data["members"].append("developer-01")
        self.save_team(data)
        self.assert_error("重复成员 ID")

    def test_role_knowledge_types_and_reference_status(self):
        data = self.project()
        for invalid in (None, [], "team-management"):
            with self.subTest(value=invalid):
                data["roleKnowledge"] = invalid
                self.save_team(data)
                self.assert_error("roleKnowledge 必须是映射")
        for invalid in (None, "team-management", [12], ["Bad-ID"]):
            with self.subTest(value=invalid):
                data["roleKnowledge"] = {"developer": invalid}
                self.save_team(data)
                self.assert_error("roleKnowledge['developer']")
        data["roleKnowledge"] = {"developer": []}
        self.save_team(data)
        self.assert_valid()
        self.definition("knowledge", "historical-method", status="deprecated")
        data["roleKnowledge"] = {"developer": ["historical-method"]}
        self.save_team(data)
        self.assert_error("引用的知识必须为 active")
        data["roleKnowledge"] = {"missing-role": []}
        self.save_team(data)
        self.assert_error("引用了不存在的岗位")

    def test_all_registered_members_require_active_roles(self):
        data = self.project()
        self.add_member(data)
        self.assert_valid()
        for status in ("draft", "deprecated"):
            with self.subTest(role_status=status):
                self.definition("roles", "developer", status=status, knowledge=["software-engineering"])
                self.assert_error("role 引用的岗位必须为 active")
        data["roleKnowledge"] = {"developer": []}
        self.save_team(data)
        self.assert_error("引用的岗位必须为 active")

    def test_active_role_requires_active_base_knowledge_even_without_members(self):
        self.definition("knowledge", "software-engineering", status="draft")
        self.assert_error("knowledge 引用的知识必须为 active")

    def test_definition_and_member_references_still_need_files(self):
        self.definition("roles", "retired-role", status="deprecated", knowledge=["missing-knowledge"])
        self.assert_error("引用了不存在的知识：missing-knowledge")
        data = self.project()
        self.add_member(data, "developer-01", "missing-role")
        self.assert_error("引用了不存在的岗位：missing-role")

    def test_managed_by_is_rejected_with_migration_message(self):
        data = self.project()
        for manager in ("super-admin-01", "developer-01", None):
            with self.subTest(manager=manager):
                data["managedBy"] = manager
                self.save_team(data)
                self.assert_error("已移除 managedBy")

    def test_super_admin_cannot_be_bound_as_member(self):
        data = self.project()
        self.add_member(data, "super-admin-01", "super-admin")
        self.assert_error("super-admin 是框架入口")
        # Even leaving a legacy shared role behind cannot restore this binding.
        self.definition("roles", "super-admin", knowledge=["team-management"])
        self.assert_error("super-admin 是框架入口")

    def test_super_admin_cannot_receive_project_role_knowledge(self):
        data = self.project()
        data["roleKnowledge"] = {"super-admin": []}
        self.save_team(data)
        self.assert_error("不能配置框架入口 super-admin 的项目岗位补充知识")
        self.definition("roles", "super-admin", knowledge=["team-management"])
        self.assert_error("不能配置框架入口 super-admin 的项目岗位补充知识")

    def test_framework_admin_requires_active_team_management_knowledge(self):
        self.definition("knowledge", "team-management", status="draft")
        self.assert_error("SUPER_ADMIN.md: 框架入口基础知识 引用的知识必须为 active")
        (self.root / "knowledge/team-management.md").unlink()
        self.assert_error("SUPER_ADMIN.md: 框架入口基础知识 引用了不存在的知识")

    def test_reject_invalid_scope_member_override_and_unknown_team_fields(self):
        data = self.project()
        member = self.add_member(data)
        for scope in (None, [], "管理", [""], ["  "], [1]):
            with self.subTest(scope=scope):
                member["scope"] = scope
                self.save_member(member)
                self.assert_error("scope 必须是非空字符串列表")
        member["scope"] = ["管理"]
        member["knowledge"] = ["team-management"]
        self.save_member(member)
        data["unknownField"] = True
        self.save_team(data)
        self.assert_error("不允许成员字段 'knowledge'")
        self.assert_error("未定义的 TEAM 字段")

    def test_schema_version_is_strict_and_rejects_versions_one_and_two(self):
        data = self.project()
        for version in (True, "3", 3.0, 1, 2, None):
            with self.subTest(version=version):
                data["schemaVersion"] = version
                self.save_team(data)
                self.assert_error("schemaVersion 必须是整数 3")
        data["schemaVersion"] = 3
        self.save_team(data)
        self.assert_valid()

    def test_removed_member_fields_are_rejected(self):
        data = self.project()
        member = self.add_member(data)
        member["status"] = "removed-field"
        self.save_member(member)
        self.assert_error("不允许成员字段 'status'")
        data["memberStates"] = []
        self.save_team(data)
        self.assert_error("未定义的 TEAM 字段：'memberStates'")

    def test_members_must_be_explicit_id_list_not_null_or_inline_mappings(self):
        data = self.project()
        for members in (None, {}, "developer-01", [{"id": "developer-01"}], [1]):
            with self.subTest(members=members):
                data["members"] = members
                self.save_team(data)
                self.assert_error("members")
        del data["members"]
        self.save_team(data)
        self.assert_error("members 必须是 ID 字符串列表")

    def test_member_files_must_exist_be_registered_and_match_id(self):
        data = self.project()
        data["members"] = ["developer-01"]
        self.save_team(data)
        self.assert_error("TEAM 登记的成员配置文件不存在")
        member = {"id": "wrong-name", "role": "developer", "scope": ["开发"]}
        self.save_member(member, filename="developer-01")
        self.assert_error("id 必须与成员文件名和 TEAM 登记的成员 ID 一致")
        member["id"] = "developer-01"
        self.save_member(member)
        self.assert_valid()
        self.save_member({**member, "id": "orphan-01"})
        self.assert_error("orphan-01.yaml: 成员配置文件未在 TEAM.members 中登记")

    def test_member_files_require_yaml_extension_and_ignore_hidden_files(self):
        self.project()
        self.write("projects/example/members/.hidden.yaml", "not: a-member\n")
        self.assert_valid()
        self.write("projects/example/members/example.yml", "id: example\n")
        self.assert_error("扩展名必须为 .yaml")
        self.assert_error("example.yml: 成员配置文件未在 TEAM.members 中登记")

    def test_nested_member_files_cannot_masquerade_as_registered_members(self):
        data = self.project()
        self.add_member(data)
        self.write("projects/example/members/.history/developer-01.yaml", "not: a-member\n")
        self.assert_valid()
        self.write("projects/example/members/nested/developer-01.yaml", "not: a-member\n")
        self.assert_error("nested/developer-01.yaml: 成员配置文件未在 TEAM.members 中登记")

    def test_member_configuration_requires_exact_fields_and_valid_types(self):
        data = self.project()
        member = self.add_member(data)
        for field in ("id", "role", "scope"):
            with self.subTest(missing=field):
                candidate = dict(member)
                del candidate[field]
                self.save_member(candidate, filename="developer-01")
                self.assert_error(f"成员配置缺少字段 {field}")
        for field, values in {
            "id": (None, 1, "../escape"),
            "role": (None, [], "../developer"),
        }.items():
            for value in values:
                with self.subTest(field=field, value=value):
                    self.save_member({**member, field: value}, filename="developer-01")
                    self.assert_error(f"{field} 必须")
        for content in ("null\n", "[]\n", "member\n"):
            self.write("projects/example/members/developer-01.yaml", content)
            self.assert_error("YAML 顶层必须是映射")

    def test_member_ids_cannot_escape_directory_or_use_arbitrary_paths(self):
        data = self.project()
        data["members"] = ["../outside", "C:/outside", "sub/member", "/absolute", "Bad-ID"]
        self.save_team(data)
        validator = Validator(self.root)
        with patch.object(validator, "check_member", wraps=validator.check_member) as check:
            validator.run()
        check.assert_not_called()
        self.assertTrue(any("members[0]" in error for error in validator.errors))

    def test_escaped_member_symlink_is_rejected_before_reading(self):
        data = self.project()
        data["members"] = ["developer-01"]
        self.save_team(data)
        member_path = self.root / "projects/example/members/developer-01.yaml"
        outside_path = self.write("outside.yaml", "must not be read\n")
        original_resolve = Path.resolve

        def simulate_symlink(path, *args, **kwargs):
            if path == member_path:
                return outside_path
            return original_resolve(path, *args, **kwargs)

        # Path-resolution mocking covers this boundary on Windows without needing
        # symlink creation privileges or platform-specific developer-mode settings.
        validator = Validator(self.root)
        with patch.object(Path, "resolve", simulate_symlink):
            with patch.object(validator, "load_yaml", wraps=validator.load_yaml) as loader:
                validator.run()
        self.assertFalse(any(call.args[0] == member_path for call in loader.call_args_list))
        self.assertTrue(any("不得越出当前项目 members 目录" in error for error in validator.errors))

    def test_escaped_members_directory_is_not_enumerated(self):
        self.project()
        members_folder = self.root / "projects/example/members"
        outside_folder = self.root / "external-members"
        original_resolve = Path.resolve

        def simulate_directory_symlink(path, *args, **kwargs):
            if path == members_folder:
                return outside_folder
            return original_resolve(path, *args, **kwargs)

        validator = Validator(self.root)
        with patch.object(Path, "resolve", simulate_directory_symlink):
            with patch("validate_framework.os.walk") as walk:
                validator.check_member_inventory(self.root / "projects/example", set())
        walk.assert_not_called()
        self.assertTrue(any("不得越出当前项目 members 目录" in error for error in validator.errors))

    def test_missing_required_files_and_teamless_project(self):
        (self.root / "SESSION_PROTOCOL.md").unlink()
        (self.root / "SUPER_ADMIN.md").unlink()
        (self.root / "MIGRATIONS.md").unlink()
        (self.root / "templates/project/TASKS.md").unlink()
        (self.root / "templates/project/members/README.md").unlink()
        (self.root / "projects/empty-project").mkdir(parents=True)
        self.assert_error("SESSION_PROTOCOL.md: 缺少必需文件")
        self.assert_error("SUPER_ADMIN.md: 缺少必需文件")
        self.assert_error("MIGRATIONS.md: 缺少必需文件")
        self.assert_error("templates/project/TASKS.md: 缺少必需文件")
        self.assert_error("templates/project/members/README.md: 缺少必需文件")
        self.assert_error("projects/empty-project/TEAM.yaml: 缺少必需文件")

    def test_markdown_links_and_ignored_examples(self):
        self.write("templates/project/AGENTS.md", "[根规则](../../AGENTS.md)\n")
        self.write("notes/with space.md", "# target\n")
        self.write("notes/with(parentheses).md", "# target\n")
        self.write("notes/linked.md", "# target\n")
        self.write("README.md", """# Links
[valid](SESSION_PROTOCOL.md#initialization)
[space](<notes/with space.md>)
[encoded](notes/with%20space.md)
[parentheses](notes/with(parentheses).md)
[title](notes/linked.md "Link title")
[source][ref]
[ref]: notes/linked.md
[web](https://example.invalid/unknown)
[mail](mailto:example@example.invalid)
[anchor](#unknown)
[dynamic](projects/{{project_id}}/TEAM.yaml)
[role](roles/<role-id>.md)
`[inline example](missing-inline.md)`
```markdown
[fenced example](missing-fenced.md)
```
~~~markdown
[tilde example](missing-tilde.md)
~~~
""")
        self.assert_valid()
        self.write("notes/broken.md", "# Broken\n\n[missing](does-not-exist.md)\n")
        self.assert_error("notes/broken.md: 第 3 行本地链接目标不存在")

    def test_frontmatter_shape_ids_and_safe_yaml(self):
        self.write("roles/developer.md", "# no frontmatter\n")
        self.assert_error("缺少 YAML 文件头")
        self.write("roles/developer.md", "---\nid: other-role\nname: Test\nstatus: active\nknowledge: []\n---\n")
        self.assert_error("id 必须与文件名一致")
        self.assert_error("knowledge 必须是非空 ID 字符串列表")
        self.write("roles/developer.md", "---\n!!python/object/apply:os.system ['echo unexpected']\n---\n")
        self.assert_error("YAML 无效")

    def test_cli_exit_status_and_read_only_behavior(self):
        before = {path.relative_to(self.root): path.read_bytes()
                  for path in self.root.rglob("*") if path.is_file()}
        with redirect_stdout(io.StringIO()) as output:
            code = main(["--root", str(self.root)])
        self.assertEqual(0, code)
        self.assertIn("校验通过", output.getvalue())
        after = {path.relative_to(self.root): path.read_bytes()
                 for path in self.root.rglob("*") if path.is_file()}
        self.assertEqual(before, after)
        (self.root / "PROJECT_PROTOCOL.md").unlink()
        result = subprocess.run(
            [sys.executable, "-B", str(Path(__file__).with_name("validate_framework.py")),
             "--root", str(self.root)],
            capture_output=True, encoding="utf-8", env={**os.environ, "PYTHONUTF8": "1"},
            check=False,
        )
        self.assertEqual(1, result.returncode, result.stderr)
        self.assertIn("PROJECT_PROTOCOL.md", result.stdout)
        self.assertIn("不验证业务语义", result.stdout)


class RealTemplateIntegrationTests(unittest.TestCase):
    def test_create_empty_project_from_current_template_then_add_members(self):
        source = Path(__file__).resolve().parents[1]
        with tempfile.TemporaryDirectory(prefix="framework-template-integration-") as temporary:
            root = Path(temporary)
            # Copy only public framework material; never read or copy real projects.
            for path in source.glob("*.md"):
                shutil.copy2(path, root / path.name)
            for name in ("roles", "knowledge", "adapters", "templates"):
                shutil.copytree(source / name, root / name)

            project = root / "projects" / "demo-project"
            shutil.copytree(root / "templates/project", project)
            replacements = {"{{project_id}}": "demo-project", "{{project_name}}": "模板建项测试"}
            seen_placeholders = set()
            for path in project.rglob("*"):
                if not path.is_file():
                    continue
                original = path.read_text(encoding="utf-8-sig")
                rendered = original
                for placeholder, value in replacements.items():
                    if placeholder in rendered:
                        seen_placeholders.add(placeholder)
                    rendered = rendered.replace(placeholder, value)
                path.write_text(rendered, encoding="utf-8")
                for placeholder in replacements:
                    self.assertNotIn(placeholder, rendered, str(path.relative_to(root)))
            self.assertEqual(set(replacements), seen_placeholders)

            validator = Validator(root)
            self.assertEqual([], validator.run())
            self.assertEqual(1, validator.project_count)
            self.assertGreater(validator.link_count, 0)
            self.assertIn("{{project_id}}", (root / "templates/project/TEAM.yaml").read_text(encoding="utf-8"))

            team_path = project / "TEAM.yaml"
            team = yaml.safe_load(team_path.read_text(encoding="utf-8"))
            self.assertEqual("demo-project", team["project"])
            self.assertEqual(3, team["schemaVersion"])
            self.assertEqual([], team["members"])
            self.assertNotIn("managedBy", team)
            self.assertEqual([], list((project / "members").glob("*.yaml")))
            # First add a directly tasked developer without a project manager,
            # then register a project manager as a separate member document.
            for role in ("developer", "project-manager"):
                identifier = f"{role}-01"
                team["members"].append(identifier)
                member = {"id": identifier, "role": role, "scope": ["执行职责范围内工作"]}
                (project / "members" / f"{identifier}.yaml").write_text(
                    yaml.safe_dump(member, sort_keys=False, allow_unicode=True), encoding="utf-8",
                )
                team_path.write_text(yaml.safe_dump(team, sort_keys=False, allow_unicode=True), encoding="utf-8")
                self.assertEqual([], Validator(root).run())


if __name__ == "__main__":
    unittest.main()
