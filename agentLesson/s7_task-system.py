"""
来源：https://github.com/shareAI-lab/learn-claude-code/blob/main/docs/zh/s07-task-system.md
"""
import json
from asyncio import tasks

# 问题描述
'''
s3的TodoManager只是内存中的扁平清单：没有顺序、没有依赖、状态只有做完没做完，真实目标是有结构的
-- 任务B 依赖任务A，任务C和D可以并行，任务E要等C和D都完成
-- 没有显示关系，Agent分不清什么能做，什么被卡住，什么能同时跑，而且清单只活在内存里，上下文压缩一跑就没有了
'''

# 解决方案
'''
把扁平清单升级为持久化到磁盘的任务图。每个任务是一个 JSON 文件, 有状态、前置依赖 (blockedBy)。任务图随时回答三个问题:
-- 1、什么可以做？ 状态为 pending且blockBy为空的任务
-- 2、什么被卡住？ 等待前置任务完成任务
-- 3、什么做完了？ 状态为completed的任务，完成时自动解锁后续任务

====================================================================================================
.tasks/
 task_1.json {"id":1, "status":"completed"}
 task_2.json {"id":2, "blockBy":[1], "status":"pending"}"
 task_3.json {"id":3, "blockBy":[1], "status":"pending"}
 task_4.json {"id":4, "blockBy":[2,3], "status":"pending"}

任务图（DAG）
                      +----------+
                +---->| Task2    |
                |     | Pending  |------+
                |     +----------+      |
   +------------+                       |    +-----------+
   | Task1      |                       +--->| Task4     |
   | completed  |                            | Blocked   |
   +------------+                       +--->|           |
                |                       |    +-----------+
                |      +----------+     |
                +----->| Task3    |-----+
                       | Pending  |
                       +----------+
                       
                       
顺序:   task 1 必须先完成, 才能开始 2 和 3
并行:   task 2 和 3 可以同时执行
依赖:   task 4 要等 2 和 3 都完成
状态:   pending -> in_progress -> completed

==========================================================================================================


'''
import json
import os
import httpx
import subprocess
from pathlib import Path

from anthropic import Anthropic


WORKDIR = Path.cwd()
TASKS_DIR = WORKDIR / ".tasks"

http_client = httpx.Client(verify=False)
client = Anthropic(api_key="sk-d6545a933eee4b429bd43c198c4026d7",
                   base_url="https://api.deepseek.com/anthropic",
                   http_client = http_client)

MODEL = 'deepseek-chat'
SYSTEM = f"You are a coding agent at {WORKDIR}. Use task tools to plan and track work."

# TaskManager :CRUD with dependency graph, persisted as JSON files
class TaskManager:
    def __init__(self, tasks_dir: Path):
        self.dir = tasks_dir
        self.dir.mkdir(parents=True, exist_ok=True)
        self._next_id = self._max_id() + 1

    def _max_id(self) -> int:
        ids = [int(f.stem.split("_")[1]) for f in self.dir.glob("task_*.json")]
        return max(ids) if ids else 0

    def _load(self, task_id: int) -> dict:
        path = self.dir.joinpath(f"task_{task_id}.json")
        if not path.exists():
            raise FileNotFoundError(f"task_{task_id} does not exist")

        return json.loads(path.read_text())

    def _save(self, task: dict) -> None:
        path = self.dir.joinpath(f"task_{task['id']}.json")
        path.write_text(json.dumps(task, indent=2, ensure_ascii=False))


    def create (self, subject:str, description:str) -> str:
        task={
            "id":self._next_id,
            "subject":subject,
            "description":description,
            "status":"pending",
            "blockedBy": [],
            "owner": "",
        }

        self._save(task)
        self._next_id += 1
        return json.dumps(task, indent=2, ensure_ascii=False)


    def get(self, task_id: int) -> str:
        return json.dumps(self._load(task_id), indent=2, ensure_ascii=False)

    def update(self, task_id: int, status:str = None, add_blocked_by:list =None, remove_blocked_by:list =None) -> str:
        task = self._load(task_id)
        if status:
           if status not in ("pending", "in_progress", "completed"):
               raise ValueError(f"Invalid status: {status}")

           task["status"] = status
           if status == "completed":
               self._clear_dependency(task_id)

        if add_blocked_by:
           task["blockedBy"] = list(set(task["blockedBy"] + add_blocked_by))

        if remove_blocked_by:
           task["blockedBy"] = [x for x in task["blockedBy"] if x not in remove_blocked_by]

        self._save(task)
        return json.dumps(task, indent=2, ensure_ascii=False)


    def _clear_dependency(self, completed_id: int):
        """Remove completed_id from all other tasks' blockedBy lists."""
        for f in self.dir.glob("task_*.json"):
            task = json.loads(f.read_text())
            if completed_id in task.get("blockedBy", []):
                task["blockedBy"].remove(completed_id)
                self._save(task)


    def list_all(self)->str:
        tasks = []
        files = sorted(self.dir.glob("task_*.json"), key=lambda f: int(f.stem.split("_")[1]))
        for f in files:
            tasks.append(json.loads(f.read_text()))
        if not tasks:
            return "No tasks."
        lines = []
        for t in tasks:
            marker = {"pending": "[ ]", "in_progress": "[>]", "completed": "[x]"}.get(t["status"], "[?]")
            blocked = f" (blocked by: {t['blockedBy']})" if t.get("blockedBy") else ""
            lines.append(f"{marker} #{t['id']}: {t['subject']}{blocked}")
        return "\n".join(lines)



TASKS = TaskManager(TASKS_DIR)


# Base Tools Implementations
def safe_path(p:str) -> Path:
    path = (WORKDIR / p).resolve()
    if not path.is_relative_to(WORKDIR):
        raise ValueError(f"Path escapes workspace: {p}")
    return path


def run_bash(command: str) -> str:
    dangerous = ["rm -rf /", "sudo", "shutdown", "reboot", "> /dev/"]
    if any(d in command for d in dangerous):
        return "Error: Dangerous command blocked"
    try:
        r = subprocess.run(command, shell=True, cwd=WORKDIR, capture_output=True, text=True, timeout=120)
        out = (r.stdout + r.stderr).strip()
        return out[:50000] if out else "(no output)"
    except subprocess.TimeoutExpired:
        return "Error: Timeout (120s)"


def run_read(path: str, limit: int = None) -> str:
    try:
        lines = safe_path(path).read_text().splitlines()
        if limit and limit < len(lines):
            lines = lines[:limit] + [f"... ({len(lines) - limit} more)"]
        return "\n".join(lines)[:50000]
    except Exception as e:
        return f"Error: {e}"

def run_write(path: str, content: str) -> str:
    try:
        fp = safe_path(path)
        fp.parent.mkdir(parents=True, exist_ok=True)
        fp.write_text(content)
        return f"Wrote {len(content)} bytes"
    except Exception as e:
        return f"Error: {e}"

def run_edit(path: str, old_text: str, new_text: str) -> str:
    try:
        fp = safe_path(path)
        c = fp.read_text()
        if old_text not in c:
            return f"Error: Text not found in {path}"
        fp.write_text(c.replace(old_text, new_text, 1))
        return f"Edited {path}"
    except Exception as e:
        return f"Error: {e}"


TOOL_HANDLERS = {
    "bash":        lambda **kw: run_bash(kw["command"]),
    "read_file":   lambda **kw: run_read(kw["path"], kw.get("limit")),
    "write_file":  lambda **kw: run_write(kw["path"], kw["content"]),
    "edit_file":   lambda **kw: run_edit(kw["path"], kw["old_text"], kw["new_text"]),
    "task_create": lambda **kw: TASKS.create(kw["subject"], kw.get("description", "")),
    "task_update": lambda **kw: TASKS.update(kw["task_id"], kw.get("status"), kw.get("addBlockedBy"), kw.get("removeBlockedBy")),
    "task_list":   lambda **kw: TASKS.list_all(),
    "task_get":    lambda **kw: TASKS.get(kw["task_id"]),
}

TOOLS = [
    {"name": "bash", "description": "Run a shell command.",
     "input_schema": {"type": "object", "properties": {"command": {"type": "string"}}, "required": ["command"]}},
    {"name": "read_file", "description": "Read file contents.",
     "input_schema": {"type": "object", "properties": {"path": {"type": "string"}, "limit": {"type": "integer"}}, "required": ["path"]}},
    {"name": "write_file", "description": "Write content to file.",
     "input_schema": {"type": "object", "properties": {"path": {"type": "string"}, "content": {"type": "string"}}, "required": ["path", "content"]}},
    {"name": "edit_file", "description": "Replace exact text in file.",
     "input_schema": {"type": "object", "properties": {"path": {"type": "string"}, "old_text": {"type": "string"}, "new_text": {"type": "string"}}, "required": ["path", "old_text", "new_text"]}},
    {"name": "task_create", "description": "Create a new task.",
     "input_schema": {"type": "object", "properties": {"subject": {"type": "string"}, "description": {"type": "string"}}, "required": ["subject"]}},
    {"name": "task_update", "description": "Update a task's status or dependencies.",
     "input_schema": {"type": "object", "properties": {"task_id": {"type": "integer"}, "status": {"type": "string", "enum": ["pending", "in_progress", "completed"]}, "addBlockedBy": {"type": "array", "items": {"type": "integer"}}, "removeBlockedBy": {"type": "array", "items": {"type": "integer"}}}, "required": ["task_id"]}},
    {"name": "task_list", "description": "List all tasks with status summary.",
     "input_schema": {"type": "object", "properties": {}}},
    {"name": "task_get", "description": "Get full details of a task by ID.",
     "input_schema": {"type": "object", "properties": {"task_id": {"type": "integer"}}, "required": ["task_id"]}},
]


def agent_loop(messages: list):
    while True:
        print("Input-To-Model :", messages)
        response = client.messages.create(
            model=MODEL,
            system=SYSTEM,
            messages=messages,
            tools=TOOLS,
            max_tokens=8000,
        )
        print("Model-Response:", response)
        messages.append({"role": "assistant", "content": response.content})
        if response.stop_reason != "tool_use":
            return
        results = []
        for block in response.content:
            if block.type == "tool_use":
                handler = TOOL_HANDLERS.get(block.name)
                try:
                    print("CALL-TOOLS ：", block)
                    output = handler(**block.input) if handler else f"Unknown tool: {block.name}"
                except Exception as e:
                    output = f"Error: {e}"
                print(f"> {block.name}:")
                print(str(output)[:200])
                results.append({"type": "tool_result", "tool_use_id": block.id, "content": str(output)})
        messages.append({"role": "user", "content": results})


if __name__ == "__main__":
    history = []
    while True:
        try:
            query = input("s07 >>")
        except (EOFError, KeyboardInterrupt):
            break
        if query.strip().lower() in ("q", "exit", ""):
            break
        history.append({"role": "user", "content": query})
        agent_loop(history)
        response_content = history[-1]["content"]
        if isinstance(response_content, list):
            for block in response_content:
                if hasattr(block, "text"):
                    print(block.text)
        print()

