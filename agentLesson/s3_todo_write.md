# 1、Use原始的输入
 
Use的原始输入
```shell
s03 >> 非上海户籍申请社保流程
```

# 2、交互流程
## 2.1、Use输入 --> Agent主程序
```shell

1、[x]:代表第几次循环
2、User_Input：代表的输入该模型的上下文


[1] >> User_Input = [{
        'role': 'user',
        'content': '非上海户籍申请社保流程'
    }
]
```
## 2.2、Agent主程序 --> 将User_Input传入模型 --> 模型反馈信息
将User_Input经过模型处理之后将回答信息反馈给Agent程序
```shell
[1] >> Model_Output = Message(
  id = '38a503e6-7c30-48f7-91f1-a62944197832',  #消息的唯一标识符（UUID 格式），用于追踪和引用这条消息
  container = None,  #消息所属的容器（如会话或对话组）。这里为 None，表示未指定容器
  content = [
      #文本块
      TextBlock(
          citations = None,  #引用来源信息（如参考文献）。这里为 None，表示没有提供引用
          text = '我来帮您了解非上海户籍申请社保的流程。首先让我创建一个任务列表来系统地收集相关信息。', #实际的文本内容。此处是 AI 助手开始回答时的说明性文字
          type = 'text' #块类型标识，固定为 text
      ),
      #工具调用块
      ToolUseBlock(
          id = 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN', #本次工具调用的唯一标识符，用于关联工具执行结果
          caller = None,  #调用工具的对象或函数名。这里为 None
          input = {  #传递给工具的参数。此处是 todo 工具需要的任务列表（每个任务有 id、text、status）
                    'items': [{
                            'id': '1',
                            'text': '搜索上海社保相关政策文件',
                            'status': 'pending'
                        }, {
                            'id': '2',
                            'text': '查找非上海户籍人员社保申请条件',
                            'status': 'pending'
                        }, {
                            'id': '3',
                            'text': '收集申请所需材料清单',
                            'status': 'pending'
                        }, {
                            'id': '4',
                            'text': '整理具体办理流程步骤',
                            'status': 'pending'
                        }, {
                            'id': '5',
                            'text': '汇总办理地点和联系方式',
                            'status': 'pending'
                        }]
                    },
          name = 'todo',  #要调用的工具名称（这里是 todo，表示创建一个待办任务列表）
          type = 'tool_use'   #块类型标识，固定为 tool_use
        )
  ],
  model = 'deepseek-chat',  #生成此消息所使用的模型名称
  role = 'assistant',  #消息的发送者角色。assistant 表示这是 AI 助手的回复
  stop_reason = 'tool_use', #消息生成结束的原因。tool_use 表示模型因为需要调用工具而停止生成（等待工具执行结果）。其他可能值：stop（自然结束）、length（达到长度限制）等
  stop_sequence = None,  #触发停止的自定义字符串。这里为 None，表示没有使用自定义停止序列
  type = 'message',  #对象的类型标识，固定为 message，表示这是一个消息对象
  usage = Usage(cache_creation = None,
                cache_creation_input_tokens = 0,  #创建缓存所消耗的 token 数。这里为 None，表示未使用缓存创建
                cache_read_input_tokens = 0,  #用于缓存创建的输入 token 数量
                inference_geo = None,  #推理服务的地理位置信息。为 None 表示未提供
                input_tokens = 644,  #本次请求中用户输入（提示词）包含的 token 总数
                output_tokens = 177,  #模型生成的回复内容所包含的 token
                server_tool_use = None,  #服务端工具使用相关的 token 计数。为 None
                service_tier = 'standard')  #服务等级（如 standard 标准级）。可能影响计费和性能
)
```

## 2.3、 Agent主程序 --> 将模型反馈信息添加到 --> User_Input
```shell
将模型反馈的 content 添加到 User_Input中

[1] >> Model_Output_To_Msg = 
 content = [
      #文本块
      TextBlock(
          citations = None,  #引用来源信息（如参考文献）。这里为 None，表示没有提供引用
          text = '我来帮您了解非上海户籍申请社保的流程。首先让我创建一个任务列表来系统地收集相关信息。', #实际的文本内容。此处是 AI 助手开始回答时的说明性文字
          type = 'text' #块类型标识，固定为 text
      ),
      #工具调用块
      ToolUseBlock(
          id = 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN', #本次工具调用的唯一标识符，用于关联工具执行结果
          caller = None,  #调用工具的对象或函数名。这里为 None
          input = {  #传递给工具的参数。此处是 todo 工具需要的任务列表（每个任务有 id、text、status）
                    'items': [{
                            'id': '1',
                            'text': '搜索上海社保相关政策文件',
                            'status': 'pending'
                        }, {
                            'id': '2',
                            'text': '查找非上海户籍人员社保申请条件',
                            'status': 'pending'
                        }, {
                            'id': '3',
                            'text': '收集申请所需材料清单',
                            'status': 'pending'
                        }, {
                            'id': '4',
                            'text': '整理具体办理流程步骤',
                            'status': 'pending'
                        }, {
                            'id': '5',
                            'text': '汇总办理地点和联系方式',
                            'status': 'pending'
                        }]
                    },
          name = 'todo',  #要调用的工具名称（这里是 todo，表示创建一个待办任务列表）
          type = 'tool_use'   #块类型标识，固定为 tool_use
        )
  ],
```
## 2.4、 Agent主程序 --> 获取待执行的tool --> tool调用
工具名称：todo
```shell
输入参数：input

>> Call_Tool_Input = ToolUseBlock(id = 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN', caller = None, input = {
            'items': [{
                    'id': '1',
                    'text': '搜索上海社保相关政策文件',
                    'status': 'pending'
                }, {
                    'id': '2',
                    'text': '查找非上海户籍人员社保申请条件',
                    'status': 'pending'
                }, {
                    'id': '3',
                    'text': '收集申请所需材料清单',
                    'status': 'pending'
                }, {
                    'id': '4',
                    'text': '整理具体办理流程步骤',
                    'status': 'pending'
                }, {
                    'id': '5',
                    'text': '汇总办理地点和联系方式',
                    'status': 'pending'
                }
            ]
        },
name = 'todo',
type = 'tool_use'):
```
工具调用的反馈结果

```shell
>> Call_Tool_Output =
    [] # 1: 搜索上海社保相关政策文件
    [] # 2: 查找非上海户籍人员社保申请条件
    [] # 3: 收集申请所需材料清单
    [] # 4: 整理具体办理流程步骤
    [] # 5: 汇总办理地点和联系方式
```
## 2.5、 tool反馈信息 --> 将反馈信息添加到 --> User_Input
tool调用之后的反馈信息，需要将其添加到User_Input中
```shell
[1] >> Tools_Call_Output_To_Msg = [{
            'type': 'tool_result',
            'tool_use_id': 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN',
            'content': '[ ] #1: 搜索上海社保相关政策文件\n[ ] #2: 查找非上海户籍人员社保申请条件\n[ ] #3: 收集申请所需材料清单\n[ ] #4: 整理具体办理流程步骤\n[ ] #5: 汇总办理地点和联系方式\n\n(0/5 completed)'
        }
    ]
```

```shell
User_Input = [
    {
      'role': 'user',
      'content': '非上海户籍申请社保流程'
    },
    {
        'role': 'assistant',
        'content': [
            TextBlock(
                  citations = None,
                  text = '我来帮您了解非上海户籍申请社保的流程。首先让我创建一个任务列表来系统地收集相关信息。',
                  type = 'text'
            ),
            ToolUseBlock(
                  id = 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN',
                  caller = None,
                  input = {
                                  'items': [{
                                          'id': '1',
                                          'text': '搜索上海社保相关政策文件',
                                          'status': 'pending'
                                      }, {
                                          'id': '2',
                                          'text': '查找非上海户籍人员社保申请条件',
                                          'status': 'pending'
                                      }, {
                                          'id': '3',
                                          'text': '收集申请所需材料清单',
                                          'status': 'pending'
                                      }, {
                                          'id': '4',
                                          'text': '整理具体办理流程步骤',
                                          'status': 'pending'
                                      }, {
                                          'id': '5',
                                          'text': '汇总办理地点和联系方式',
                                          'status': 'pending'
                                      }
                                  ]
                              },
                  name = 'todo',
                  type = 'tool_use'
            )
         ]
    },
    {
        'role': 'user',
        'content': [{
            'type': 'tool_result',
            'tool_use_id': 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN',
            'content': '[ ] #1: 搜索上海社保相关政策文件\n[ ] #2: 查找非上海户籍人员社保申请条件\n[ ] #3: 收集申请所需材料清单\n[ ] #4: 整理具体办理流程步骤\n[ ] #5: 汇总办理地点和联系方式\n\n(0/5 completed)'
          }]
    }
]
```

## 2.6、小结：交互流程图如下
第一次交互的流程图如下，后续会循环这个流程，每次通过修改User_Input来实现多轮对话
```shell

       用户             Agent               Model            Tools
---------------    --------------     --------------    --------------
       |                  |                  |                 |
       | User_Input       |                  |                 |
       |----------------->|                  |                 |                 
       |                  | User_Input       |                 |
       |                  |----------------->|                 |                 
       |                  |                  |                 |
       |                  | Response Message |                 |                 
       |                  |<-----------------|                 |
       |                  |                  |                 |
       | RM add to        |                  |                 |
       | User_Input       |                  |                 |
       |<---------------- |                  |                 |
       |                  |                  |                 |
       |                  |  Call tool       |                 |
       |                  |----------------------------------->| 
       |                  |                  |                 |
       |                  |  Response call tool                |
       |                  |<-----------------------------------|
       | RC add to        |                  |                 |
       | User_Input       |                  |                 |
       |<-----------------|                  |                 |
       |                  |                  |                 |
       |                  |                  |                 |


```

## 2.7、第二次交互流程
```shell
# User_Input  --> Agent主程序
[2] >> User_Input = [{
            'role': 'user',
            'content': '非上海户籍申请社保流程'
        }, {
            'role': 'assistant',
            'content': [
                       TextBlock(citations = None, text = '我来帮您了解非上海户籍申请社保的流程。首先让我创建一个任务列表来系统地收集相关信息。', type = 'text'),
                       ToolUseBlock(id = 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN', caller = None, input = {
                        'items': [{
                                'id': '1',
                                'text': '搜索上海社保相关政策文件',
                                'status': 'pending'
                            }, {
                                'id': '2',
                                'text': '查找非上海户籍人员社保申请条件',
                                'status': 'pending'
                            }, {
                                'id': '3',
                                'text': '收集申请所需材料清单',
                                'status': 'pending'
                            }, {
                                'id': '4',
                                'text': '整理具体办理流程步骤',
                                'status': 'pending'
                            }, {
                                'id': '5',
                                'text': '汇总办理地点和联系方式',
                                'status': 'pending'
                            }
                        ]
                    }, name = 'todo', type = 'tool_use')]
        }, {
            'role': 'user',
            'content': [{
                    'type': 'tool_result',
                    'tool_use_id': 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN',
                    'content': '[ ] #1: 搜索上海社保相关政策文件\n[ ] #2: 查找非上海户籍人员社保申请条件\n[ ] #3: 收集申请所需材料清单\n[ ] #4: 整理具体办理流程步骤\n[ ] #5: 汇总办理地点和联系方式\n\n(0/5 completed)'
                }
            ]
        }
    ]

# Agent主程序 --> 模型
    [2] >> Model_Output = Message(id = '2dbd014a-5f38-452f-87ee-a29daeec11ed', container = None,
               content = [TextBlock(citations = None, text = '现在让我开始搜索相关信息。首先搜索上海社保相关政策文件。', type = 'text'),
                         ToolUseBlock(id = 'call_00_k8FtWv1yLX6yI8mYprZqEeyH', caller = None, input = {
                    'items': [{
                            'id': '1',
                            'text': '搜索上海社保相关政策文件',
                            'status': 'in_progress'
                        }, {
                            'id': '2',
                            'text': '查找非上海户籍人员社保申请条件',
                            'status': 'pending'
                        }, {
                            'id': '3',
                            'text': '收集申请所需材料清单',
                            'status': 'pending'
                        }, {
                            'id': '4',
                            'text': '整理具体办理流程步骤',
                            'status': 'pending'
                        }, {
                            'id': '5',
                            'text': '汇总办理地点和联系方式',
                            'status': 'pending'
                        }
                    ]
                }, name = 'todo', type = 'tool_use')], model = 'deepseek-chat', role = 'assistant', stop_reason = 'tool_use', stop_sequence = None, type = 'message',
          usage = Usage(cache_creation = None, cache_creation_input_tokens = 0, cache_read_input_tokens = 0, inference_geo = None, input_tokens = 880,
output_tokens = 145, server_tool_use = None, service_tier = 'standard'))

# Agent主程序 --> 将模型反馈信息添加到 --> UserInput
    [2] >> Model_Output_To_Msg = [
TextBlock(citations = None, text = '现在让我开始搜索相关信息。首先搜索上海社保相关政策文件。', type = 'text'),
ToolUseBlock(id = 'call_00_k8FtWv1yLX6yI8mYprZqEeyH', caller = None, input = {
                'items': [{
                        'id': '1',
                        'text': '搜索上海社保相关政策文件',
                        'status': 'in_progress'
                    }, {
                        'id': '2',
                        'text': '查找非上海户籍人员社保申请条件',
                        'status': 'pending'
                    }, {
                        'id': '3',
                        'text': '收集申请所需材料清单',
                        'status': 'pending'
                    }, {
                        'id': '4',
                        'text': '整理具体办理流程步骤',
                        'status': 'pending'
                    }, {
                        'id': '5',
                        'text': '汇总办理地点和联系方式',
                        'status': 'pending'
                    }
                ]
            }, name = 'todo', type = 'tool_use')]

# Agent主程序 --> 调度工具

     # 工具函数入参信息
     >> TodoManager_Input_param = [{
            'id': '1',
            'text': '搜索上海社保相关政策文件',
            'status': 'in_progress'
        }, {
            'id': '2',
            'text': '查找非上海户籍人员社保申请条件',
            'status': 'pending'
        }, {
            'id': '3',
            'text': '收集申请所需材料清单',
            'status': 'pending'
        }, {
            'id': '4',
            'text': '整理具体办理流程步骤',
            'status': 'pending'
        }, {
            'id': '5',
            'text': '汇总办理地点和联系方式',
            'status': 'pending'
        }
    ]

     # 调用的工具信息
     >> Call_Tool_Input = ToolUseBlock(id = 'call_00_k8FtWv1yLX6yI8mYprZqEeyH', caller = None, input = {
            'items': [{
                    'id': '1',
                    'text': '搜索上海社保相关政策文件',
                    'status': 'in_progress'
                }, {
                    'id': '2',
                    'text': '查找非上海户籍人员社保申请条件',
                    'status': 'pending'
                }, {
                    'id': '3',
                    'text': '收集申请所需材料清单',
                    'status': 'pending'
                }, {
                    'id': '4',
                    'text': '整理具体办理流程步骤',
                    'status': 'pending'
                }, {
                    'id': '5',
                    'text': '汇总办理地点和联系方式',
                    'status': 'pending'
                }
            ]
        }, name = 'todo', type = 'tool_use'):

    # 工具函数的反馈信息
     >> Call_Tool_Output = [ > ] # 1: 搜索上海社保相关政策文件
    [] # 2: 查找非上海户籍人员社保申请条件
    [] # 3: 收集申请所需材料清单
    [] # 4: 整理具体办理流程步骤
    [] # 5: 汇总办理地点和联系方式

    (0 / 5 completed)

    # 工具的函数反馈信息 --> 将其此信息添加到 --> UserInput
    [2] >> Tools_Call_Output_To_Msg = [{
            'type': 'tool_result',
            'tool_use_id': 'call_00_k8FtWv1yLX6yI8mYprZqEeyH',
            'content': '[>] #1: 搜索上海社保相关政策文件\n[ ] #2: 查找非上海户籍人员社保申请条件\n[ ] #3: 收集申请所需材料清单\n[ ] #4: 整理具体办理流程步骤\n[ ] #5: 汇总办理地点和联系方式\n\n(0/5 completed)'
        }
    ]
```

## 2.8、第三次次交互流程
```shell

# User_Input的输入信息
[3] >> User_Input = [{
            'role': 'user',
            'content': '非上海户籍申请社保流程'
        }, {
            'role': 'assistant',
            'content': [
TextBlock(citations = None, text = '我来帮您了解非上海户籍申请社保的流程。首先让我创建一个任务列表来系统地收集相关信息。', type = 'text'),
ToolUseBlock(id = 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN', caller = None, input = {
                        'items': [{
                                'id': '1',
                                'text': '搜索上海社保相关政策文件',
                                'status': 'pending'
                            }, {
                                'id': '2',
                                'text': '查找非上海户籍人员社保申请条件',
                                'status': 'pending'
                            }, {
                                'id': '3',
                                'text': '收集申请所需材料清单',
                                'status': 'pending'
                            }, {
                                'id': '4',
                                'text': '整理具体办理流程步骤',
                                'status': 'pending'
                            }, {
                                'id': '5',
                                'text': '汇总办理地点和联系方式',
                                'status': 'pending'
                            }
                        ]
                    }, name = 'todo', type = 'tool_use')]
        }, {
            'role': 'user',
            'content': [{
                    'type': 'tool_result',
                    'tool_use_id': 'call_00_RDtM3NbpQn81fqZ2VXx3G3DN',
                    'content': '[ ] #1: 搜索上海社保相关政策文件\n[ ] #2: 查找非上海户籍人员社保申请条件\n[ ] #3: 收集申请所需材料清单\n[ ] #4: 整理具体办理流程步骤\n[ ] #5: 汇总办理地点和联系方式\n\n(0/5 completed)'
                }
            ]
        }, {
            'role': 'assistant',
            'content': [
TextBlock(citations = None, text = '现在让我开始搜索相关信息。首先搜索上海社保相关政策文件。', type = 'text'),
ToolUseBlock(id = 'call_00_k8FtWv1yLX6yI8mYprZqEeyH', caller = None, input = {
                        'items': [{
                                'id': '1',
                                'text': '搜索上海社保相关政策文件',
                                'status': 'in_progress'
                            }, {
                                'id': '2',
                                'text': '查找非上海户籍人员社保申请条件',
                                'status': 'pending'
                            }, {
                                'id': '3',
                                'text': '收集申请所需材料清单',
                                'status': 'pending'
                            }, {
                                'id': '4',
                                'text': '整理具体办理流程步骤',
                                'status': 'pending'
                            }, {
                                'id': '5',
                                'text': '汇总办理地点和联系方式',
                                'status': 'pending'
                            }
                        ]
                    }, name = 'todo', type = 'tool_use')]
        }, {
            'role': 'user',
            'content': [{
                    'type': 'tool_result',
                    'tool_use_id': 'call_00_k8FtWv1yLX6yI8mYprZqEeyH',
                    'content': '[>] #1: 搜索上海社保相关政策文件\n[ ] #2: 查找非上海户籍人员社保申请条件\n[ ] #3: 收集申请所需材料清单\n[ ] #4: 整理具体办理流程步骤\n[ ] #5: 汇总办理地点和联系方式\n\n(0/5 completed)'
                }
            ]
        }
    ]
```
