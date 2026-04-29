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
