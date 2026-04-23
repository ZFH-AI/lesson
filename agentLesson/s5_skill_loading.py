import os
import re
import httpx
import subprocess
from dataclasses import dataclass
from http import client
from pathlib import Path
from anthropic import Anthropic
from pygments.lexers import shell


http_client = httpx.Client(verify=False)
client = Anthropic(api_key="sk-d6545a933eee4b429bd43c198c4026d7",
                   base_url="https://api.deepseek.com/anthropic",
                   http_client = http_client)

MODEL = 'deepseek-chat'

@dataclass
class SkillManifest:
    name: str
    description: str
    path: Path


@dataclass
class SkillDocument:
    manifest: SkillManifest
    body: str

# skill注册、批量加载skill全文
class SkillRegistry:
    def __init__(self, skills_dir:Path):
      self.skills_dir = skills_dir
      self.documents: dict[str,SkillDocument] = {}
      self._load_all()

    def _load_all(self):
     if not os.path.exists(self.skills_dir):
         return

     path_list = sorted(self.skills_dir.rglob("SKILL.md"))
     for path in path_list:
        print(f"Loading {path}")
        # 按照skill的路径读取skill的内容，将元数据保存在meta中，其他完整信息保存在body中
        meta, body = self._parse_frontmatter(path.read_text(encoding='utf-8'))
        # 元数据处理
        name = meta.get("name", path.parent.name)
        description = meta.get("description", "No description")
        # 保存元数据的数据类
        manifest = SkillManifest(name=name, description=description, path=path)
        # 元数据的数据集合
        self.documents[name] = SkillDocument(manifest=manifest, body=body.strip())

    def _parse_frontmatter(self, text:str) -> tuple[dict, str]:
          match = re.match(r"^---\n(.*?)\n---\n(.*)", text, re.DOTALL)
          if not match:
              return {}, text

          meta = {}
          for line in match.group(1).strip().splitlines():
              if ":" not in line:
                  continue
              key, value = line.split(":", 1)
              meta[key.strip()] = value.strip()

          return meta, match.group(2)

    # 获得skills目录下的所有skill的元数据描述
    def describe_available(self) ->str:
        if not self.documents:
          return "(no skills available)"

        lines = []
        for name in sorted(self.documents):
          manifest = self.documents[name].manifest
          lines.append(f"- {manifest.name}: {manifest.description}")
        return "\n".join(lines)

    def load_full_text(self, name: str) -> str:
        document = self.documents.get(name)
        if not document:
            known = ", ".join(sorted(self.documents)) or "(none)"
            return f"Error: Unknown skill '{name}'. Available skills: {known}"

        return (
            f"<skill name=\"{document.manifest.name}\">\n"
            f"{document.body}\n"
            "</skill>"
        )

WORKDIR = Path.cwd()
SKILLS_DIR = WORKDIR / "skills"
SKILL_REGISTRY = SkillRegistry(SKILLS_DIR)
# -- 测试skill加载
# print(SKILL_REGISTRY.load_full_text("抖音神曲收集"))


SYSTEM = f"""You are a coding agent at {WORKDIR}.
Use load_skill when a task needs specialized instructions before you act.

Skills available:
{SKILL_REGISTRY.describe_available()}
"""

# 路径处理
def safe_path(path_str: str) -> Path:
    path = (WORKDIR / path_str).resolve()
    if not path.is_relative_to(WORKDIR):
        raise ValueError(f"Path escapes workspace: {path_str}")
    return path

# bash执行的函数
def run_bash(command:str) -> str:
    dangerous = ['rm -rf /', 'sudo', 'shutdown', 'reboot', '> /dev/']

    if any(x in command for x in dangerous):
        return "Error: Dangerous command blocked"

    try:
        result  = subprocess.run(
            command,
            shell=True,
            cwd=WORKDIR,
            capture_output=True,
            text=True,
            timeout=120,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
    except subprocess.TimeoutExpired:
        return "Error: Timeout (120s)"

    output = (result.stdout + result.stderr).strip()
    return output[:50000] if output else "(no output)"

# 读文件，行数限制可选
def run_read(path_str:str, limit:int | None = None) -> str:
    try:
        lines = safe_path(path_str).read_text("utf-8").splitlines()
        if limit and limit < len(lines):
            lines = lines[:limit] + [f"... ({len(lines) - limit} more lines)"]

        return "\n".join(lines)[:50000]

    except Exception as exc:
        return f"Error: {exc}"

def run_write(path_str:str, content:str) -> str:
    try:
        file_path = safe_path(path_str)
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(content, encoding="utf-8")
        return f"Wrote {len(content)} bytes to {path_str}"
    except Exception as exc:
        return f"Error: {exc}"

def run_edit(path_str:str, old_text:str, new_text:str) -> str:
    try:
        file_path = safe_path(path_str)
        content = file_path.read_text(encoding="utf-8")
        if old_text not in content:
            return f"Error: Text not found in {path_str}"

        file_path.write_text(content.replace(old_text, new_text, 1))

        return f"Edited {path_str}"

    except Exception as exc:
        return f"Error: {exc}"


# 工具函数
TOOL_HANDLERS = {
    "bash": lambda **kw: run_bash(kw["command"]),
    "read_file": lambda **kw: run_read(kw["path"], kw.get("limit")),
    "write_file": lambda **kw: run_write(kw["path"], kw["content"]),
    "edit_file": lambda **kw: run_edit(kw["path"], kw["old_text"], kw["new_text"]),
    "load_skill": lambda **kw: SKILL_REGISTRY.load_full_text(kw["name"]),
}


#输入的格式要求
TOOLS = [
    {"name": "bash", "description": "Run a shell command.",
     "input_schema": {"type": "object", "properties": {"command": {"type": "string"}}, "required": ["command"]}},
    {"name": "read_file", "description": "Read file contents.",
     "input_schema": {"type": "object", "properties": {"path": {"type": "string"}, "limit": {"type": "integer"}}, "required": ["path"]}},
    {"name": "write_file", "description": "Write content to file.",
     "input_schema": {"type": "object", "properties": {"path": {"type": "string"}, "content": {"type": "string"}}, "required": ["path", "content"]}},
    {"name": "edit_file", "description": "Replace exact text in file.",
     "input_schema": {"type": "object", "properties": {"path": {"type": "string"}, "old_text": {"type": "string"}, "new_text": {"type": "string"}}, "required": ["path", "old_text", "new_text"]}},
    {"name": "load_skill", "description": "Load specialized knowledge by name.",
     "input_schema": {"type": "object", "properties": {"name": {"type": "string", "description": "Skill name to load"}}, "required": ["name"]}},
]

def extract_text(content)->str:
    if not isinstance(content, list):
        return ""
    texts = []
    for block in content:
        text = getattr(block, "text", None)
        if text:
            texts.append(text)

    return "\n".join(texts).strip()


def agent_loop(messages:list) -> None:
    while True:
        print("系统提示词:", SYSTEM)
        print("Input:", messages)
        response = client.messages.create(
            model=MODEL,
            system=SYSTEM,
            messages=messages,
            tools=TOOLS,
            max_tokens=8000,
        )
        messages.append({"role": "assistant", "content": response.content})
        print("Model-Output:", response)
        if response.stop_reason != "tool_use":
            return

        results = []
        for block in response.content:
            if block.type != "tool_use":
                continue

            handler = TOOL_HANDLERS.get(block.name)
            try:
                print("Tools_Param:", block)
                output = handler(**block.input) if handler else f"Unknown tool: {block.name}"
            except Exception as exc:
                output = f"Error: {exc}"

            print(f"> {block.name}: {str(output)[:200]}")
            results.append({
                "type": "tool_result",
                "tool_use_id": block.id,
                "content": str(output),
            })

        messages.append({"role": "user", "content": results})

if __name__ == "__main__":
    history = []
    while True:
        try:
            query = input("s5 >： ")
        except (EOFError, KeyboardInterrupt):
            break
        if query.strip().lower() in ("q", "exit", ""):
            break

        history.append({"role": "user", "content": query})
        agent_loop(history)

        final_text = extract_text(history[-1]["content"])
        if final_text:
            print(final_text)
        print()

