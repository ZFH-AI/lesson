Harness 是agent在特定领域工作所需要的一切

```shell
Harness = Tools + Knowledge + Observation + Action Interfaces + Permissions

1、Tools            --> 文件读写，shell脚本、网络、数据库、浏览器

2、Knowledge        --> 产品文档、领域资料、API规范、风格指南

3、Observation      --> Git Diff、错误日志、浏览器状态、传感器数据

4、Action Interface --> CLI命令、API调用、UI交互

5、Permission       --> 沙箱隔离、审批流程、信任边界
```

Harness 工程师做什么

```shell
1、实现工具         --> 给agent一双手，文件读写、shell执行、API调用、浏览器控制、数据库查询、每个工具都是Agent在环境中
                       可以采取的一个行动，设计它们时要原子化、可组合、描述清晰

2、策划知识         --> 给agent领域专长，产品文档、架构决策记录、风格指南、合规要求，按需加载，不要前置塞入，agent应该
                       知道有什么可用，然后给自己拉取所需

3、管理上下文       --> 给agent干净的记忆，子agent隔离，防止噪声泄露，上下文压缩方式历史淹没，任务系统让目标持久化到单次
                       对话

4、控制权限         -->  给 agent 边界。沙箱化文件访问。对破坏性操作要求审批。在 agent 和外部系统之间实施信任边界。这是安全
                        工程与 harness 工程的交汇点

5、收集任务过程数据  -->  Agent 在你的 harness 中执行的每一条行动序列都是训练信号。真实部署中的感知-推理-行动轨迹是微调下一代 
                        agent 模型的原材料。你的 harness 不仅服务于 agent -- 它还可以帮助进化 agent

```

本课程流程

主流程
```shell

The agent pattern
=================

User --> messages[] --> LLM --> response
          ^                          |
          |                 stop_reason == "tool_use"
          |                 /                   \
          |                yes                  No
          |   1、执行工具：execute tools          1、return text
          |   2、添加反馈：append results
          +-- 3、LOOP

这是个最小的循环，每个AI Agent都需要这个循环
1、模型决定何时调用工具，何时停止
2、代码只是执行模型的要求

核心模式
================
def agent_loop(messages):
    while True:
        response = client.messages.create(
            model=MODEL, system=SYSTEM,
            messages=messages, tools=TOOLS,
        )
        messages.append({"role": "assistant",
                         "content": response.content})

        if response.stop_reason != "tool_use":
            return

        results = []
        for block in response.content:
            if block.type == "tool_use":
                output = TOOL_HANDLERS[block.name](**block.input)
                results.append({
                    "type": "tool_result",
                    "tool_use_id": block.id,
                    "content": output,
                })
        messages.append({"role": "user", "content": results})


```
课程表
```shell
s01   "One loop & Bash is all you need" — 一个工具 + 一个循环 = 一个 Agent

s02   "加一个工具, 只加一个 handler" — 循环不用动, 新工具注册进 dispatch map 就行

s03   "没有计划的 agent 走哪算哪" — 先列步骤再动手, 完成率翻倍

s04   "大任务拆小, 每个小任务干净的上下文" — Subagent 用独立 messages[], 不污染主对话

s05   "用到什么知识, 临时加载什么知识" — 通过 tool_result 注入, 不塞 system prompt

s06   "上下文总会满, 要有办法腾地方" — 三层压缩策略, 换来无限会话

s07   "大目标要拆成小任务, 排好序, 记在磁盘上" — 文件持久化的任务图, 为多 agent 协作打基础

s08   "慢操作丢后台, agent 继续想下一步" — 后台线程跑命令, 完成后注入通知

s09   "任务太大一个人干不完, 要能分给队友" — 持久化队友 + 异步邮箱

s10   "队友之间要有统一的沟通规矩" — 一个 request-response 模式驱动所有协商

s11   "队友自己看看板, 有活就认领" — 不需要领导逐个分配, 自组织

s12   "各干各的目录, 互不干扰" — 任务管目标, worktree 管目录, 按 ID 绑定
```

