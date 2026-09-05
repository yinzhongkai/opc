#!/usr/bin/env python3
"""Read-only validation of the collaboration framework's files and references.

This checks configuration and ordinary local Markdown links, not business
correctness, task acceptance, session identity, or authorization.
"""

from __future__ import annotations

import argparse
import os
from pathlib import Path
import re
import sys
from urllib.parse import unquote

sys.dont_write_bytecode = True
if __name__ == "__main__":
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")
try:
    import yaml
    if not hasattr(yaml, "SafeLoader"):
        raise ImportError("PyYAML could not be loaded")
except ImportError:
    raise SystemExit(
        "无法加载依赖 PyYAML。请检查安装与读取权限；安装命令："
        "python -m pip install -r scripts/requirements.txt"
    ) from None


ID_PATTERN = re.compile(r"[a-z][a-z0-9-]*\Z")
DEFINITION_STATES = {"draft", "active", "deprecated"}
ROOT_FILES = (
    "README.md", "AGENTS.md", "SESSION_PROTOCOL.md", "PROJECT_PROTOCOL.md",
    "CONFIG_SCHEMA.md", "SUPER_ADMIN.md", "MIGRATIONS.md",
)
PROJECT_FILES = (
    "AGENTS.md", "PROJECT.md", "TEAM.yaml", "TASKS.md", "STATUS.md",
    "DECISIONS.md", "HANDOFFS.md", "artifacts/README.md", "members/README.md",
)
PROJECT_PLACEHOLDERS = ("{{project_id}}", "{{project_name}}")
TEAM_FIELDS = {
    "schemaVersion", "project", "members", "roleKnowledge",
}
MEMBER_FIELDS = {"id", "role", "scope"}


class UniqueKeyLoader(yaml.SafeLoader):
    """Safe YAML loader that rejects duplicate mapping keys at every depth."""

    def construct_mapping(self, node, deep=False):
        if not isinstance(node, yaml.MappingNode):
            return super().construct_mapping(node, deep=deep)
        self.flatten_mapping(node)
        mapping = {}
        for key_node, value_node in node.value:
            key = self.construct_object(key_node, deep=deep)
            try:
                repeated = key in mapping
            except TypeError:
                raise yaml.constructor.ConstructorError(
                    "读取映射时", node.start_mark, "映射键必须是可哈希的标量",
                    key_node.start_mark,
                ) from None
            if repeated:
                raise yaml.constructor.ConstructorError(
                    "读取映射时", node.start_mark, f"重复 YAML 键：{key!r}",
                    key_node.start_mark,
                )
            mapping[key] = self.construct_object(value_node, deep=deep)
        return mapping


def is_id(value):
    return isinstance(value, str) and ID_PATTERN.fullmatch(value) is not None


def is_text(value):
    return isinstance(value, str) and bool(value.strip())


def without_code(text):
    """Mask fenced blocks and inline code, preserving line numbers."""
    output = []
    fence = None
    for line in text.splitlines(keepends=True):
        marker = re.match(r"^ {0,3}(`{3,}|~{3,})(.*)$", line.rstrip("\r\n"))
        if fence:
            output.append(re.sub(r"[^\r\n]", " ", line))
            if (marker and marker[1][0] == fence[0]
                    and len(marker[1]) >= fence[1] and not marker[2].strip()):
                fence = None
        elif marker:
            fence = (marker[1][0], len(marker[1]))
            output.append(re.sub(r"[^\r\n]", " ", line))
        else:
            output.append(line)
    return re.sub(
        r"(`+)(?!`)(.*?)\1(?!`)",
        lambda match: re.sub(r"[^\r\n]", " ", match[0]),
        "".join(output), flags=re.DOTALL,
    )


def markdown_targets(text):
    """Yield (target, line) for ordinary inline links and link definitions."""
    clean = without_code(text)
    patterns = (
        r"!?\[[^\]\n]*\]\(\s*(<[^>\n]+>|(?:\\.|[^()\n]|\([^()\n]*\))+?)\s*\)",
        r"^ {0,3}\[[^\]\n]+\]:\s*(<[^>\n]+>|\S+)",
    )
    for pattern in patterns:
        for match in re.finditer(pattern, clean, flags=re.MULTILINE):
            destination = match[1].strip()
            if destination.startswith("<") and destination.endswith(">"):
                destination = destination[1:-1]
            elif destination.startswith("<") and ">" in destination:
                destination = destination[1:destination.index(">")]
            else:
                destination = re.split(r"\s", destination, maxsplit=1)[0]
            yield destination, clean.count("\n", 0, match.start()) + 1


class Validator:
    def __init__(self, root):
        self.root = Path(root).resolve()
        self.errors = []
        self.roles = {}
        self.knowledge = {}
        self.project_count = 0
        self.link_count = 0

    def error(self, path, message):
        try:
            location = Path(path).relative_to(self.root).as_posix()
        except ValueError:
            location = str(path)
        self.errors.append(f"{location}: {message}")

    def read(self, path):
        try:
            return path.read_text(encoding="utf-8-sig")
        except (OSError, UnicodeError) as exc:
            self.error(path, f"无法读取 UTF-8 文本：{exc}")
            return None

    def load_yaml(self, path, frontmatter=False):
        text = self.read(path)
        if text is None:
            return None
        if frontmatter:
            lines = text.splitlines()
            if not lines or lines[0].strip() != "---":
                self.error(path, "缺少 YAML 文件头（第一行应为 ---）")
                return None
            end = next((i for i in range(1, len(lines)) if lines[i].strip() == "---"), None)
            if end is None:
                self.error(path, "YAML 文件头缺少结束标记 ---")
                return None
            text = "\n".join(lines[1:end])
        try:
            data = yaml.load(text, Loader=UniqueKeyLoader)
        except yaml.YAMLError as exc:
            mark = getattr(exc, "problem_mark", None)
            line = f"第 {mark.line + (2 if frontmatter else 1)} 行：" if mark else ""
            self.error(path, f"YAML 无效：{line}{getattr(exc, 'problem', None) or exc}")
            return None
        if not isinstance(data, dict):
            self.error(path, "YAML 顶层必须是映射")
            return None
        return data

    def id_list(self, value, path, field, nonempty=False):
        if not isinstance(value, list) or (nonempty and not value):
            self.error(path, f"{field} 必须是{'非空' if nonempty else ''} ID 字符串列表")
            return []
        valid = []
        for index, item in enumerate(value):
            if not is_id(item):
                self.error(path, f"{field}[{index}] 必须是小写字母开头的 ID")
            else:
                valid.append(item)
        return valid

    def require_files(self, folder, names):
        for name in names:
            path = folder / name
            if not path.is_file():
                self.error(path, "缺少必需文件")

    def load_definitions(self, folder_name, role=False):
        folder = self.root / folder_name
        if not folder.is_dir():
            self.error(folder, "缺少共享定义目录")
            return {}
        definitions = {}
        # Enumerate explicitly so .MD/.Md files are not silently skipped on
        # case-sensitive systems or accepted by Windows glob matching.
        for path in sorted(folder.iterdir()):
            if not path.is_file() or path.suffix.lower() != ".md":
                continue
            if path.name.upper() == "INDEX.MD" or path.name.startswith("."):
                continue
            if path.suffix != ".md":
                self.error(path, "定义文件扩展名必须为小写 .md")
            data = self.load_yaml(path, frontmatter=True)
            if data is None:
                continue
            identifier = data.get("id")
            if not is_id(identifier):
                self.error(path, "id 必须是小写字母开头的 ID")
            elif identifier != path.stem:
                self.error(path, "id 必须与文件名一致")
            if not is_text(data.get("name")):
                self.error(path, "name 必须是非空字符串")
            if not isinstance(data.get("status"), str) or data["status"] not in DEFINITION_STATES:
                self.error(path, "status 必须是 draft、active 或 deprecated")
            if role:
                data["knowledge"] = self.id_list(
                    data.get("knowledge"), path, "knowledge", nonempty=True,
                )
            if is_id(identifier):
                if identifier in definitions:
                    self.error(path, f"重复定义 ID：{identifier}")
                else:
                    definitions[identifier] = (data, path)
        return definitions

    def check_knowledge_ref(self, identifier, path, field, active):
        entry = self.knowledge.get(identifier)
        if entry is None:
            self.error(path, f"{field} 引用了不存在的知识：{identifier}")
        elif active and entry[0].get("status") != "active":
            self.error(path, f"{field} 引用的知识必须为 active：{identifier}")

    def member_path_is_safe(self, path, folder):
        """Reject escaped member references before reading their contents."""
        expected_root = folder.resolve() / "members"
        try:
            resolved = path.resolve()
        except (OSError, RuntimeError) as exc:
            self.error(path, f"无法解析成员路径：{exc}")
            return False
        if not resolved.is_relative_to(expected_root):
            self.error(path, "成员配置路径不得越出当前项目 members 目录（包括符号链接）")
            return False
        return True

    def check_member(self, folder, identifier):
        path = folder / "members" / f"{identifier}.yaml"
        if not self.member_path_is_safe(path, folder):
            return
        if not path.is_file():
            self.error(path, "TEAM 登记的成员配置文件不存在")
            return
        member = self.load_yaml(path)
        if member is None:
            return
        for key in MEMBER_FIELDS - member.keys():
            self.error(path, f"成员配置缺少字段 {key}")
        for key in member.keys() - MEMBER_FIELDS:
            self.error(path, f"不允许成员字段 {key!r}")
        if not is_id(member.get("id")):
            self.error(path, "id 必须是小写字母开头的 ID")
        elif member["id"] != identifier:
            self.error(path, "id 必须与成员文件名和 TEAM 登记的成员 ID 一致")
        scope = member.get("scope")
        if not isinstance(scope, list) or not scope or any(not is_text(item) for item in scope):
            self.error(path, "scope 必须是非空字符串列表，每项不得留空")
        role_id = member.get("role")
        if not is_id(role_id):
            self.error(path, "role 必须是有效岗位 ID")
            return
        if role_id == "super-admin":
            self.error(path, "super-admin 是框架入口，不能绑定为项目成员岗位")
        entry = self.roles.get(role_id)
        if entry is None:
            self.error(path, f"role 引用了不存在的岗位：{role_id}")
        elif entry[0].get("status") != "active":
            self.error(path, f"role 引用的岗位必须为 active：{role_id}")

    def check_member_inventory(self, folder, registered, template=False):
        members_folder = folder / "members"
        if not self.member_path_is_safe(members_folder, folder) or not members_folder.is_dir():
            return
        for directory, dirs, names in os.walk(members_folder, followlinks=False):
            dirs[:] = sorted(name for name in dirs if not name.startswith("."))
            for name in sorted(names):
                path = Path(directory) / name
                if name.startswith(".") or path.suffix.lower() not in {".yaml", ".yml"}:
                    continue
                if template:
                    self.error(path, "空白模板不能预置成员配置文件")
                if path.suffix != ".yaml":
                    self.error(path, "成员配置文件扩展名必须为 .yaml（不接受 .yml 或大写扩展名）")
                if (path.parent != members_folder or path.suffix != ".yaml"
                        or path.stem not in registered):
                    self.error(path, "成员配置文件未在 TEAM.members 中登记")

    def check_project_placeholders(self, folder):
        """Check framework project records, not arbitrary business templates."""
        for name in PROJECT_FILES:
            path = folder / name
            if not path.is_file():
                continue
            text = self.read(path)
            if text is None:
                continue
            # Include code spans/blocks: the project entry's ID is inline code.
            for line, content in enumerate(text.splitlines(), start=1):
                for placeholder in PROJECT_PLACEHOLDERS:
                    if placeholder in content:
                        self.error(path, f"第 {line} 行存在未替换的项目占位符：{placeholder}")

    def check_team(self, folder, template=False):
        self.require_files(folder, PROJECT_FILES)
        if not template:
            self.check_project_placeholders(folder)
        path = folder / "TEAM.yaml"
        if not path.is_file():
            return
        data = self.load_yaml(path)
        if data is None:
            return
        for key in data:
            if key not in TEAM_FIELDS:
                if key == "managedBy":
                    self.error(path, "schemaVersion 3 已移除 managedBy；超级管理员是框架入口，请迁移旧配置")
                else:
                    self.error(path, f"未定义的 TEAM 字段：{key!r}")
        if type(data.get("schemaVersion")) is not int or data["schemaVersion"] != 3:
            self.error(path, "schemaVersion 必须是整数 3；旧版配置请按 MIGRATIONS.md 迁移")
        project = data.get("project")
        if template:
            if project != "{{project_id}}":
                self.error(path, "空白模板的 project 必须保留 {{project_id}} 占位符")
        elif not is_id(project):
            self.error(path, "project 必须是小写字母开头的 ID（真实项目不能使用占位符）")
        elif project != folder.name:
            self.error(path, f"project 必须与项目目录名一致：{folder.name}")
        role_knowledge = data.get("roleKnowledge", {})
        if not isinstance(role_knowledge, dict):
            self.error(path, "roleKnowledge 必须是映射，省略时默认为 {}")
        else:
            for role_id, knowledge_ids in role_knowledge.items():
                field = f"roleKnowledge[{role_id!r}]"
                if not is_id(role_id):
                    self.error(path, f"{field} 的岗位键必须是有效 ID")
                else:
                    if role_id == "super-admin":
                        self.error(path, f"{field} 不能配置框架入口 super-admin 的项目岗位补充知识")
                    entry = self.roles.get(role_id)
                    if entry is None:
                        self.error(path, f"{field} 引用了不存在的岗位")
                    elif entry[0].get("status") != "active":
                        self.error(path, f"{field} 引用的岗位必须为 active")
                for identifier in self.id_list(knowledge_ids, path, field):
                    self.check_knowledge_ref(identifier, path, field, active=True)
        if template and data.get("members") != []:
            self.error(path, "空白模板的 members 必须为空列表")
        members = self.id_list(data.get("members"), path, "members")
        registered = set()
        for identifier in members:
            if identifier in registered:
                self.error(path, f"重复成员 ID：{identifier}")
                continue
            registered.add(identifier)
            self.check_member(folder, identifier)
        self.check_member_inventory(folder, registered, template=template)

    def check_markdown_links(self):
        for folder, dirs, names in os.walk(self.root, followlinks=False):
            dirs[:] = sorted(name for name in dirs if not name.startswith("."))
            for name in sorted(names):
                if not name.lower().endswith(".md") or name.startswith("."):
                    continue
                path = Path(folder) / name
                text = self.read(path)
                if text is None:
                    continue
                for target, line in markdown_targets(text):
                    if (not target or target.startswith(("#", "//"))
                            or re.match(r"^[A-Za-z][A-Za-z0-9+.-]*:", target)):
                        continue
                    target = unquote(re.split(r"[?#]", target, maxsplit=1)[0])
                    target = re.sub(r"\\([\\`*{}\[\]()#+.!<> _-])", r"\1", target)
                    if not target or "{{" in target or re.search(r"<[^>]+>", target):
                        continue
                    self.link_count += 1
                    if not (path.parent / target).exists():
                        self.error(path, f"第 {line} 行本地链接目标不存在：{target}")

    def run(self):
        if not self.root.is_dir():
            self.error(self.root, "框架根目录不存在或不是目录")
            return self.errors
        self.require_files(self.root, ROOT_FILES)
        self.knowledge = self.load_definitions("knowledge")
        self.roles = self.load_definitions("roles", role=True)
        self.check_knowledge_ref(
            "team-management", self.root / "SUPER_ADMIN.md", "框架入口基础知识", active=True,
        )
        for role, path in self.roles.values():
            for identifier in role["knowledge"]:
                self.check_knowledge_ref(
                    identifier, path, "knowledge", active=role.get("status") == "active",
                )
        self.check_team(self.root / "templates" / "project", template=True)
        projects = self.root / "projects"
        if projects.exists() and not projects.is_dir():
            self.error(projects, "projects 必须是目录（可为空或不存在）")
        elif projects.is_dir():
            for folder in sorted(projects.iterdir()):
                if folder.name.startswith(".") or not folder.is_dir():
                    continue
                self.project_count += 1
                self.check_team(folder)
        self.check_markdown_links()
        return self.errors


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="只读检查框架配置、文件引用和普通 Markdown 本地链接；不验证业务语义或权限。",
    )
    parser.add_argument(
        "--root", type=Path, default=Path(__file__).resolve().parents[1],
        help="框架根目录；默认使用本脚本所在框架",
    )
    args = parser.parse_args(argv)
    validator = Validator(args.root)
    errors = validator.run()
    if errors:
        print(f"校验未通过：发现 {len(errors)} 项问题。")
        for error in errors:
            print(f"- {error}")
    else:
        print(
            f"校验通过：{len(validator.roles)} 个岗位、{len(validator.knowledge)} 份知识、"
            f"{validator.project_count} 个实际项目、1 套项目模板、"
            f"{validator.link_count} 处本地链接。"
        )
    print("本工具只读；不验证业务语义、任务验收、会话身份或执行权限。")
    return 1 if errors else 0


if __name__ == "__main__":
    raise SystemExit(main())
