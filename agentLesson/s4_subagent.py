"""
来源：https://github.com/shareAI-lab/learn-claude-code/blob/main/agents/s04_subagent.py


spawn a child agent with fresh messages=[]. The child works in its own xontext,
sharing the filesystem, then returns only a summary to the parent.

Parent agent                               Subagent
+--------------------------+              +-----------------------+
| messages = [...]         |              | messages = []         |<---- fresh
|                          | dispatch     |                       |
| tool:mask                |----------->  | while tool_use:       |
|  prompt = "..."          |              |   call tools          |
|  description = "..."     |              |   append results      |
|                          |              |                       |
|  return = "..."          | summary      | return last text      |
|                          |<----------   |                       |
+--------------------------+              +-----------------------+
        |
        |
        V
parent context stays clean.
subagent context is discarded


Key Insight: "Fresh messages=[] gives context isolation. The parent stays clean"

Note: Real Claude Code also use in-process isolation (not-os-level process forking).
The child runs in the same process with a fresh message array and isolated tool context 
-- same pattern as this teaching implementation.

    Comparison with real Claude Code:
    +-------------------+------------------+----------------------------------+
    | Aspect            | This demo        | Real Claude Code                 |
    +-------------------+------------------+----------------------------------+
    | Backend           | in-process only  | 5 backends: in-process, tmux,    |
    |                   |                  | iTerm2, fork, remote             |
    | Context isolation | fresh messages=[]| createSubagentContext() isolates  |
    |                   |                  | ~20 fields (tools, permissions,  |
    |                   |                  | cwd, env, hooks, etc.)           |
    | Tool filtering    | manually curated | resolveAgentTools() filters from |
    |                   |                  | parent pool; allowedTools         |
    |                   |                  | replaces all allow rules         |
    | Agent definition  | hardcoded system | .claude/agents/*.md with YAML    |
    |                   | prompt           | frontmatter (AgentTemplate)      |
    +-------------------+------------------+----------------------------------+


"""
