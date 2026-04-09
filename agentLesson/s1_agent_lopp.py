# 特别说明，本系列所有的源码来源：https://github.com/shareAI-lab/learn-claude-code/tree/main/agents
#           如有违权联系立删 
#

# !/usr/bin/env python3
# Harness: the loop -- the model's first connection to the real world.
"""
s01_agent_loop.py - The Agent Loop

The entire secret of an AI coding agent in one pattern:

    while stop_reason == "tool_use":
        response = LLM(messages, tools)
        execute tools
        append results

    +----------+      +-------+      +---------+
    |   User   | ---> |  LLM  | ---> |  Tool   |
    |  prompt  |      |       |      | execute |
    +----------+      +---+---+      +----+----+
                          ^               |
                          |   tool_result |
                          +---------------+
                          (loop continues)

This is the core loop: feed tool results back to the model
until the model decides to stop. Production agents layer
policy, hooks, and lifecycle controls on top.
"""

import os
import subprocess
import httpx
from anthropic import Anthropic
from dotenv import load_dotenv
from matplotlib.backend_tools import ToolXScale

load_dotenv(override=True)

if os.getenv("ANTHROPIC_BASE_URL"):
    os.environ.pop("ANTHROPIC_AUTH_TOKEN", None)

http_client = httpx.Client(verify=False)
client = Anthropic(api_key="XX",
                   base_url="XX",
                   http_client = http_client)
MODEL = 'deepseek-chat'

SYSTEM = f"You are a coding agent at {os.getcwd()}. Use bash to solve tasks. Act, don't explain."

TOOLS = [{
    "name":"bash",
    "description":"Run a shell command",
    "input_schema":{
        "type": "object",
        "properties": {"command": {"type": "string"}},
        "required": ["command"]
    },
}]

def run_bash(command: str):
    dangerous = ["rm -rf / ", "sudo", "shutdown", "rebot", "> /dev/"]
    if any(d in command for d in dangerous):
        return "Error：Dangerous command blocked"

    try:
        r = subprocess.run(command, shell = True, cwd = os.getcwd(),
                           capture_output = True, text = True, timeout = 120)

        out = (r.stdout + r.stderr).strip()
        return out[:50000] if out else "(no output)"
    except subprocess.TimeoutExpired:
        return "Error: Timeout (120s)"



def agent_loop(messages:list):
    while True:
        responses = client.messages.create(
            model = MODEL,
            system = SYSTEM,
            messages = messages,
            tools = TOOLS,
            max_tokens = 8000,
        )

        messages.append({
            "role": "assistant",
            "content": responses.content
        })

        if responses.stop_reason != "tools_use":
            return

        results = []
        for block in responses.content:
            if block.type == "tool_use":
                print(f"\033[33m$ {block.input['command']}\033[0m")
                output = run_bash(block.input["command"])
                print(output[:200])
                results.append({"type": "tool_result", "tool_use_id": block.id,
                                "content": output})

        messages.append({"role": "user", "content": results})


if __name__ == "__main__":
    history = []

    while True:
        try:
            query = input("\033[36ms01 >> \033[0m")
        except (EOFError, KeyboardInterrupt):
            break
        if query.strip().lower() in ("q", "exit", ""):
            break

        history.append({
            "role":"user",
            "content":query})

        agent_loop(history)

        response_content = history[-1]["content"]
        if isinstance(response_content, list):
            for block in response_content:
                if hasattr(block, "text"):
                    print(block.text)
        print()
