"""
来源：https://github.com/shareAI-lab/learn-claude-code/blob/main/agents/s06_context_compact.py
"""
import json
import os
import subprocess
import time
import httpx
from pathlib import Path
from anthropic import Anthropic

"""
Three-layer compression pipeline so the agent can work forever

   Every Turn:
   +---------------------+
   | Tool call result    |
   +---------------------+
            |
            V
    Layer 1 : Micro_compact (silent every turn)
    +---------------------------------------------------+
    | replace non-read_file tool_result content older   |
    | than 3 with "[Previous: used {tool_name}]"        |
    +---------------------------------------------------+
            |
            V
    +---------------------------+
    | Check: Tokens > 50000 ?   |
    +---------------------------+
        |             |
        no           yes
        |             |
        V             V
    continue      Layer 2 : auto_compact
                  +-----------------------------------------------+
                  | Save full transcript to .transcripts/         |
                  | Ask LLM to summerize conversation             |
                  | Replace all message with [summaey]            |
                  +-----------------------------------------------+
                                     |
                                     V
                  Layer 3 : compact tool
                  +-------------------------------------------------+
                  | Model calls compact -> immediate summarization  |
                  | Same as auto , triggered manually               |
                  +-------------------------------------------------+
                  
Key insight :"The agent can forget strategically and keep working forever"
             
"""

http_client = httpx.Client(verify=False)
client = Anthropic(api_key="sk-xx",
                   base_url="https://api.deepseek.com/anthropic",
                   http_client = http_client)

MODEL = 'deepseek-chat'
WORKDIR = Path.cwd()
SYSTEM = f"You are a coding agent at {WORKDIR}. Use tools to solve tasks."

THRESHOLD = 50000
TRANSCRIPT_DIR = WORKDIR / ".transcripts"
KEEP_RECENT = 3
PRESERVE_RESULT_TOOLS = {"read_file"}


# Layer 1 ： micro_compact replace old tool results witch placeholders
def micro_compact(messages:list) -> list:
    # 收集所有 角色为为user的信息
    tool_results = []
    for msg_idx, msg in enumerate(messages):
        if msg["role"] == "user" and isinstance(msg.get("content"), list):
            for part_idx, part in enumerate(msg["content"]):
                if isinstance(part, dict) and part.get("type") == "tool_result":
                    tool_results.append((msg_idx, part_idx, part))

    if len(tool_results) <= KEEP_RECENT:
        return  messages

    # 收集所有 角色为assistant 使用的工具名称（模型调用过的工具名称）
    tool_name_map = {}
    for msg in messages:
        if msg["role"] == "assistant":
            content = msg.get("content", [])
            if isinstance(content, list):
                for block in content:
                    if hasattr(block, "type") and block.type == "tool_use":
                        tool_name_map[block.id] = block.name

    # 返回前 N-KEEP_RECENT次结果，比如交互10次，会返回前7次的交互信息
    to_clear = tool_results[:-KEEP_RECENT]
    for _,_, result in to_clear:
        if not isinstance(result.get("content"), str) or len(result.get("content")) <= 100:
            continue
        tool_id = result.get("tool_use_id", "")
        tool_name = tool_name_map.get(tool_id, "unknown")
        if tool_name in PRESERVE_RESULT_TOOLS:
            continue
        result["content"] = f"[Previous: used {tool_name}]"
    return messages


# Layer 2 ： auto_compact save transcript, summarize, replace message
def auto_compact(messages:list) -> list:

    # 将上下文信息保存到磁盘中
    TRANSCRIPT_DIR.mkdir(exist_ok=True)
    transcripts_path = TRANSCRIPT_DIR / f"transcripts_{int(time.time())}.jsonl"
    with open(transcripts_path, "w") as f:
        for msg in messages:
            f.write(json.dumps(msg, default=str) + "\n")

    print(f"[transcript saved: {transcripts_path}]")

    # 询问LLM生成摘要
    conversation_text = json.dumps(messages, default=str)[-80000:]
    response = client.messages.create(
        model=MODEL,
        messages=[{"role": "user", "content":
            "Summarize this conversation for continuity. Include: "
            "1) What was accomplished, 2) Current state, 3) Key decisions made. "
            "Be concise but preserve critical details.\n\n" + conversation_text}],
        max_tokens=2000,
    )
    summary = next((block.text for block in response.content if hasattr(block, "text")), "")
    if not summary:
        summary = "No summary generated."
    # Replace all messages with compressed summary
    return [
        {"role": "user", "content": f"[Conversation compressed. Transcript: {transcripts_path}]\n\n{summary}"},
    ]

# tools implementations
def safe_path(p: str) -> Path:
    path = (WORKDIR / p).resolve()
    if not path.is_relative_to(WORKDIR):
        raise ValueError(f"Path escapes workspace: {p}")
    return path

def run_bash(command: str) -> str:
    dangerous = ["rm -rf /", "sudo", "shutdown", "reboot", "> /dev/"]
    if any(d in command for d in dangerous):
        return "Error: Dangerous command blocked"
    try:
        r = subprocess.run(command, shell=True, cwd=WORKDIR,
                           capture_output=True, text=True, timeout=120)
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
        content = fp.read_text()
        if old_text not in content:
            return f"Error: Text not found in {path}"
        fp.write_text(content.replace(old_text, new_text, 1))
        return f"Edited {path}"
    except Exception as e:
        return f"Error: {e}"

TOOL_HANDLERS = {
    "bash":       lambda **kw: run_bash(kw["command"]),
    "read_file":  lambda **kw: run_read(kw["path"], kw.get("limit")),
    "write_file": lambda **kw: run_write(kw["path"], kw["content"]),
    "edit_file":  lambda **kw: run_edit(kw["path"], kw["old_text"], kw["new_text"]),
    "compact":    lambda **kw: "Manual compression requested.",
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
    {"name": "compact", "description": "Trigger manual conversation compression.",
     "input_schema": {"type": "object", "properties": {"focus": {"type": "string", "description": "What to preserve in the summary"}}}},
]


def estimate_tokens(messages: list) -> int:
    """Rough token count: ~4 chars per token."""
    return len(str(messages)) // 4

def agent_loop(messages:list):
    while True:
        # Layer 1 micro_compact before each LLM call
        micro_compact(messages)

        # Layer 2 auto_compact if token estimate exceeds threshold
        if estimate_tokens(messages) > THRESHOLD:
            print("[auto_compact triggered]")
            messages[:] = auto_compact(messages)

        # 模型调用
        response = client.messages.create(
            model=MODEL,
            system=SYSTEM,
            messages=messages,
            tools=TOOLS,
            max_tokens=8000,
        )

        # 将模型反馈的内容添加到prompt中，作为下一轮与模型交互的输入信息
        messages.append({"role": "assistant", "content": response.content})

        # 判断是否是模型反馈类型，不是直接返回，是继续执行
        if response.stop_reason != "tool_use":
            return

        # 解析模型的反馈信息，根据反馈block.name信息调用相应的tool
        results = []
        manual_compact = False
        for block in response.content:
            if block.type == "tool_use":
                # 开始模型上下文压缩
                if block.name == "compact":
                    manual_compact = True
                    output = "Compressing..."
                else:
                    # 调用tool
                    handler = TOOL_HANDLERS.get(block.name)
                    try:
                        output = handler(**block.input) if handler else f"Unknown tool: {block.name}"
                    except Exception as e:
                        output = f"Error: {e}"
                print(f"> {block.name}:")
                print(str(output)[:200])
                results.append({"type": "tool_result", "tool_use_id": block.id, "content": str(output)})

        # 将执行tool的反馈信息，作为user的信息，添加到prompt中
        messages.append({"role": "user", "content": results})

        # Layer 3: manual compact triggered by the compact tool
        if manual_compact:
            print("[manual compact]")
            messages[:] = auto_compact(messages)
            return


if __name__ == "__main__":
    history = []
    while True:
        try:
            query = input("\033[36ms06 >> \033[0m")
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
