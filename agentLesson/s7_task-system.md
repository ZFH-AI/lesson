# 1、背景说明
对s7_task-system.py的流程解读

# 2、流程解读

## 2.1 第一次交互 

1、User -->Agent主程序 ---> 模型

【User:用户原始输入】
```json
Create 3 tasks: "Setup project", "Write code", "Write tests". Make them depend on each other in order 
```

【User -> Agent主程序】
```json
Input - To - Model:
[{
        'role': 'user',
        'content': 'Create 3 tasks: "Setup project", "Write code", "Write tests". Make them depend on each other in order'
}]
```

【Agent主程序 --> 模型 --> response】
模型回复的要创建三个子流程，使用的task_create工具
```json
Model - Response: 
Message(
id = '3ac8acba-f9f7-4603-a644-2657196eaa37',
container = None, 
content = [
            ToolUseBlock(id = 'call_00_WXmTiBalRKWz3EYmlARZeQMr', caller = None, input = {
                'subject': 'Setup project',
                'description': 'Initialize the project structure, install dependencies, and configure the build system.'
            }, name = 'task_create', type = 'tool_use'),

            ToolUseBlock(id = 'call_01_EqIKuPLsCJA51gAupGihdHWi', caller = None, input = {
                'subject': 'Write code',
                'description': 'Implement the core functionality and features for the project.'
            }, name = 'task_create', type = 'tool_use'),

            ToolUseBlock(id = 'call_02_4dmnYK6Os4akzONUN8sibRdF', caller = None, input = {
                'subject': 'Write tests',
                'description': 'Create unit tests and integration tests to verify the code works correctly.'
            }, name = 'task_create', type = 'tool_use')
          ], 
model = 'deepseek-chat',
role = 'assistant',
stop_details = None,
stop_reason = 'tool_use',
stop_sequence = None,
type = 'message', 
usage = Usage(cache_creation = None, cache_creation_input_tokens = 0, cache_read_input_tokens = 640, inference_geo = None,
              input_tokens = 119, output_tokens = 185, server_tool_use = None, service_tier = 'standard'))
```

【response --> add to user input】
```json
 messages.append({"role": "assistant", "content": response.content})
```
【Agent --> call Tools --> add tools return  to user input】
```json
 CALL-TOOLS ：
        ToolUseBlock(id='call_00_WXmTiBalRKWz3EYmlARZeQMr', caller=None,
                     input={'subject': 'Setup project', 'description': 'Initialize the project structure, install dependencies, and configure the build system.'},
                     name='task_create', type='tool_use')
 TOOLS-Response:
        {
          "id": 4,
          "subject": "Setup project",
          "description": "Initialize the project structure, install dependencies, and configure the build system.",
          "status": "pending",
          "blockedBy": [],
          "owne":""
      }
 /// 
  CALL-TOOLS ：
          ToolUseBlock(id='call_01_EqIKuPLsCJA51gAupGihdHWi', caller=None,
                        input={'subject': 'Write code', 'description': 'Implement the core functionality and features for the project.'},
                        name='task_create', type='tool_use')
 TOOLS-Response:
        {
          "id": 5,
          "subject": "Write code",
          "description": "Implement the core functionality and features for the project.",
          "status": "pending",
          "blockedBy": [],
          "owner": ""
        }
  ///
CALL-TOOLS ：
      ToolUseBlock(id='call_02_4dmnYK6Os4akzONUN8sibRdF', caller=None,
                  input={'subject': 'Write tests', 'description': 'Create unit tests and integration tests to verify the code works correctly.'},
                  name='task_create', type='tool_use')
 TOOLS-Response:
      {
        "id": 6,
        "subject": "Write tests",
        "description": "Create unit tests and integration tests to verify the code works correctly.",
        "status": "pending",
        "blockedBy": [],
        "owner": ""
      }


## 将tools的反馈结果增加到user的输入中
results.append({"type": "tool_result", "tool_use_id": block.id, "content": str(output)})
messages.append({"role": "user", "content": results})

```


## 2.2  第二次交互
【User -> Agent主程序】
```json
Input - To - Model:
[
  {
          'role': 'user',
          'content': 'Create 3 tasks: "Setup project", "Write code", "Write tests". Make them depend on each other in order'
  }, 
  {
          'role': 'assistant',
          'content': [ToolUseBlock(id = 'call_00_WXmTiBalRKWz3EYmlARZeQMr', caller = None, input = {
                      'subject': 'Setup project',
                      'description': 'Initialize the project structure, install dependencies, and configure the build system.'
                  }, name = 'task_create', type = 'tool_use'), 
                     ToolUseBlock(id = 'call_01_EqIKuPLsCJA51gAupGihdHWi', caller = None, input = {
                      'subject': 'Write code',
                      'description': 'Implement the core functionality and features for the project.'
                  }, name = 'task_create', type = 'tool_use'), 
                     ToolUseBlock(id = 'call_02_4dmnYK6Os4akzONUN8sibRdF', caller = None, input = {
                      'subject': 'Write tests',
                      'description': 'Create unit tests and integration tests to verify the code works correctly.'
                  }, name = 'task_create', type = 'tool_use')]
  }, 
  {
          'role': 'user',
          'content': [{
                  'type': 'tool_result',
                  'tool_use_id': 'call_00_WXmTiBalRKWz3EYmlARZeQMr',
                  'content': '{\n  "id": 4,\n  "subject": "Setup project",\n  "description": "Initialize the project structure, install dependencies, and configure the build system.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }, {
                  'type': 'tool_result',
                  'tool_use_id': 'call_01_EqIKuPLsCJA51gAupGihdHWi',
                  'content': '{\n  "id": 5,\n  "subject": "Write code",\n  "description": "Implement the core functionality and features for the project.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }, {
                  'type': 'tool_result',
                  'tool_use_id': 'call_02_4dmnYK6Os4akzONUN8sibRdF',
                  'content': '{\n  "id": 6,\n  "subject": "Write tests",\n  "description": "Create unit tests and integration tests to verify the code works correctly.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }
          ]
  }
]
```

【Agent主程序 --> 模型 --> response】

```json
Model - Response:
Message(
id = '840dd12e-10e6-41f6-b9be-5e946a735154',
container = None, 
content = [TextBlock(citations = None, text = 'Now I\'ll set up the dependencies so they chain in order:\n\n1. "Setup project" → no dependencies (starts first)\n2. "Write code" → depends on "Setup project"\n3. "Write tests" → depends on "Write code"', type = 'text'), 
           ToolUseBlock(id = 'call_00_qHhSwaS7L0B4bRX2Nf5ndEed', caller = None, input = {
                'task_id': 5,
                'addBlockedBy': [4]
            }, name = 'task_update', type = 'tool_use'), 
           ToolUseBlock(id = 'call_01_hDB5c58yGCZ0WLs0x6AkBrkB', caller = None, input = {
                'task_id': 6,
                'addBlockedBy': [5]
            }, name = 'task_update', type = 'tool_use')], 
model = 'deepseek-chat',
role = 'assistant',
stop_details = None,
stop_reason = 'tool_use',
stop_sequence = None,
type = 'message', 
usage = Usage(cache_creation = None, cache_creation_input_tokens = 0, cache_read_input_tokens = 896, inference_geo = None, input_tokens = 233,
              output_tokens = 169, server_tool_use = None, service_tier = 'standard'))
```

【response --> add to user input】
```json
 messages.append({"role": "assistant", "content": response.content})
```

【Agent --> call Tools --> add tools return  to user input】
```json
 CALL-TOOLS ：
        ToolUseBlock(id='call_00_qHhSwaS7L0B4bRX2Nf5ndEed', caller=None,
                     input={'task_id': 5, 'addBlockedBy': [4]}, name='task_update', type='tool_use')
 TOOLS-Response:
        {
        "id": 5,
        "subject": "Write code",
        "description": "Implement the core functionality and features for the project.",
        "status": "pending",
        "blockedBy": [4],
        "owner": ""
      }
 /// 
  CALL-TOOLS ：
          ToolUseBlock(id='call_01_hDB5c58yGCZ0WLs0x6AkBrkB', caller=None,
                       input={'task_id': 6, 'addBlockedBy': [5]}, name='task_update', type='tool_use')
 TOOLS-Response:
        {
        "id": 6,
        "subject": "Write tests",
        "description": "Create unit tests and integration tests to verify the code works correctly.",
        "status": "pending",
        "blockedBy": [ 5],
        "owner": ""
        }

## 将tools的反馈结果增加到user的输入中
results.append({"type": "tool_result", "tool_use_id": block.id, "content": str(output)})
messages.append({"role": "user", "content": results})

```

## 2.3  第三次交互

【User -> Agent主程序】
```json
Input - To - Model:
[
  {
          'role': 'user',
          'content': 'Create 3 tasks: "Setup project", "Write code", "Write tests". Make them depend on each other in order'
  }, 
  {
          'role': 'assistant',
          'content': [ToolUseBlock(id = 'call_00_WXmTiBalRKWz3EYmlARZeQMr', caller = None, input = {
                      'subject': 'Setup project',
                      'description': 'Initialize the project structure, install dependencies, and configure the build system.'
                  }, name = 'task_create', type = 'tool_use'), 
                     ToolUseBlock(id = 'call_01_EqIKuPLsCJA51gAupGihdHWi', caller = None, input = {
                      'subject': 'Write code',
                      'description': 'Implement the core functionality and features for the project.'
                  }, name = 'task_create', type = 'tool_use'), 
                     ToolUseBlock(id = 'call_02_4dmnYK6Os4akzONUN8sibRdF', caller = None, input = {
                      'subject': 'Write tests',
                      'description': 'Create unit tests and integration tests to verify the code works correctly.'
                  }, name = 'task_create', type = 'tool_use')]
  }, 
  {
          'role': 'user',
          'content': [{
                  'type': 'tool_result',
                  'tool_use_id': 'call_00_WXmTiBalRKWz3EYmlARZeQMr',
                  'content': '{\n  "id": 4,\n  "subject": "Setup project",\n  "description": "Initialize the project structure, install dependencies, and configure the build system.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }, {
                  'type': 'tool_result',
                  'tool_use_id': 'call_01_EqIKuPLsCJA51gAupGihdHWi',
                  'content': '{\n  "id": 5,\n  "subject": "Write code",\n  "description": "Implement the core functionality and features for the project.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }, {
                  'type': 'tool_result',
                  'tool_use_id': 'call_02_4dmnYK6Os4akzONUN8sibRdF',
                  'content': '{\n  "id": 6,\n  "subject": "Write tests",\n  "description": "Create unit tests and integration tests to verify the code works correctly.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }
          ]
  },
  {
        'role': 'assistant',
        'content': [TextBlock(citations = None, text = 'Now I\'ll set up the dependencies so they chain in order:\n\n1. "Setup project" → no dependencies (starts first)\n2. "Write code" → depends on "Setup project"\n3. "Write tests" → depends on "Write code"', type = 'text'), ToolUseBlock(id = 'call_00_qHhSwaS7L0B4bRX2Nf5ndEed', caller = None, input = {
                    'task_id': 5,
                    'addBlockedBy': [4]
                }, name = 'task_update', type = 'tool_use'), ToolUseBlock(id = 'call_01_hDB5c58yGCZ0WLs0x6AkBrkB', caller = None, input = {
                    'task_id': 6,
                    'addBlockedBy': [5]
                }, name = 'task_update', type = 'tool_use')]
    },
    {
        'role': 'user',
        'content': [{
                'type': 'tool_result',
                'tool_use_id': 'call_00_qHhSwaS7L0B4bRX2Nf5ndEed',
                'content': '{\n  "id": 5,\n  "subject": "Write code",\n  "description": "Implement the core functionality and features for the project.",\n  "status": "pending",\n  "blockedBy": [\n    4\n  ],\n  "owner": ""\n}'
            }, {
                'type': 'tool_result',
                'tool_use_id': 'call_01_hDB5c58yGCZ0WLs0x6AkBrkB',
                'content': '{\n  "id": 6,\n  "subject": "Write tests",\n  "description": "Create unit tests and integration tests to verify the code works correctly.",\n  "status": "pending",\n  "blockedBy": [\n    5\n  ],\n  "owner": ""\n}'
            }
        ]
    }
]
```

【Agent主程序 --> 模型 --> response】
```json
Model - Response: 
Message(
id = 'cc59d705-44d6-4b31-b4d0-65f776e23532',
container = None, 
content = [ TextBlock(citations = None, text = 'Let me verify the final state:', type = 'text'), 
            ToolUseBlock(id = 'call_00_ibwEoGTSBk09HN3tY7mZ8HDk', caller = None, input = {}, name = 'task_list', type = 'tool_use')], 
model = 'deepseek-chat',
role = 'assistant',
stop_details = None,
stop_reason = 'tool_use', 
stop_sequence = None, type = 'message', 
usage = Usage(cache_creation = None, cache_creation_input_tokens = 0, cache_read_input_tokens = 1024, inference_geo = None,
               input_tokens = 407, output_tokens = 33, server_tool_use = None, service_tier = 'standard'))

```

【response --> add to user input】
```json
 messages.append({"role": "assistant", "content": response.content})
```

【Agent --> call Tools --> add tools return  to user input】
```json
CALL-TOOLS ：
      ToolUseBlock(id='call_00_ibwEoGTSBk09HN3tY7mZ8HDk', caller=None, input={}, name='task_list', type='tool_use')

TOOLS-Response:
  [ ] #4: Setup project
  [ ] #5: Write code (blocked by: [4])
  [ ] #6: Write tests (blocked by: [5])
      
## 将tools的反馈结果增加到user的输入中
results.append({"type": "tool_result", "tool_use_id": block.id, "content": str(output)})
messages.append({"role": "user", "content": results})
```

## 2.4 第四次交互
【User -> Agent主程序】
```json
Input - To - Model:
[
  {
          'role': 'user',
          'content': 'Create 3 tasks: "Setup project", "Write code", "Write tests". Make them depend on each other in order'
  }, 
  {
          'role': 'assistant',
          'content': [ToolUseBlock(id = 'call_00_WXmTiBalRKWz3EYmlARZeQMr', caller = None, input = {
                      'subject': 'Setup project',
                      'description': 'Initialize the project structure, install dependencies, and configure the build system.'
                  }, name = 'task_create', type = 'tool_use'), 
                     ToolUseBlock(id = 'call_01_EqIKuPLsCJA51gAupGihdHWi', caller = None, input = {
                      'subject': 'Write code',
                      'description': 'Implement the core functionality and features for the project.'
                  }, name = 'task_create', type = 'tool_use'), 
                     ToolUseBlock(id = 'call_02_4dmnYK6Os4akzONUN8sibRdF', caller = None, input = {
                      'subject': 'Write tests',
                      'description': 'Create unit tests and integration tests to verify the code works correctly.'
                  }, name = 'task_create', type = 'tool_use')]
  }, 
  {
          'role': 'user',
          'content': [{
                  'type': 'tool_result',
                  'tool_use_id': 'call_00_WXmTiBalRKWz3EYmlARZeQMr',
                  'content': '{\n  "id": 4,\n  "subject": "Setup project",\n  "description": "Initialize the project structure, install dependencies, and configure the build system.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }, {
                  'type': 'tool_result',
                  'tool_use_id': 'call_01_EqIKuPLsCJA51gAupGihdHWi',
                  'content': '{\n  "id": 5,\n  "subject": "Write code",\n  "description": "Implement the core functionality and features for the project.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }, {
                  'type': 'tool_result',
                  'tool_use_id': 'call_02_4dmnYK6Os4akzONUN8sibRdF',
                  'content': '{\n  "id": 6,\n  "subject": "Write tests",\n  "description": "Create unit tests and integration tests to verify the code works correctly.",\n  "status": "pending",\n  "blockedBy": [],\n  "owner": ""\n}'
              }
          ]
  },
  {
        'role': 'assistant',
        'content': [TextBlock(citations = None, text = 'Now I\'ll set up the dependencies so they chain in order:\n\n1. "Setup project" → no dependencies (starts first)\n2. "Write code" → depends on "Setup project"\n3. "Write tests" → depends on "Write code"', type = 'text'), ToolUseBlock(id = 'call_00_qHhSwaS7L0B4bRX2Nf5ndEed', caller = None, input = {
                    'task_id': 5,
                    'addBlockedBy': [4]
                }, name = 'task_update', type = 'tool_use'), ToolUseBlock(id = 'call_01_hDB5c58yGCZ0WLs0x6AkBrkB', caller = None, input = {
                    'task_id': 6,
                    'addBlockedBy': [5]
                }, name = 'task_update', type = 'tool_use')]
    },
    {
        'role': 'user',
        'content': [{
                'type': 'tool_result',
                'tool_use_id': 'call_00_qHhSwaS7L0B4bRX2Nf5ndEed',
                'content': '{\n  "id": 5,\n  "subject": "Write code",\n  "description": "Implement the core functionality and features for the project.",\n  "status": "pending",\n  "blockedBy": [\n    4\n  ],\n  "owner": ""\n}'
            }, {
                'type': 'tool_result',
                'tool_use_id': 'call_01_hDB5c58yGCZ0WLs0x6AkBrkB',
                'content': '{\n  "id": 6,\n  "subject": "Write tests",\n  "description": "Create unit tests and integration tests to verify the code works correctly.",\n  "status": "pending",\n  "blockedBy": [\n    5\n  ],\n  "owner": ""\n}'
            }
        ]
    },
    {
        'role': 'assistant',
        'content': [TextBlock(citations = None, text = 'Let me verify the final state:', type = 'text'),
                    ToolUseBlock(id = 'call_00_ibwEoGTSBk09HN3tY7mZ8HDk', caller = None, input = {}, name = 'task_list', type = 'tool_use')]
    },
    {
        'role': 'user',
        'content': [{
                'type': 'tool_result',
                'tool_use_id': 'call_00_ibwEoGTSBk09HN3tY7mZ8HDk',
                'content': '[ ] #4: Setup project\n[ ] #5: Write code (blocked by: [4])\n[ ] #6: Write tests (blocked by: [5])'
            }
        ]
    }
]
```

【Agent主程序 --> 模型 --> response】
```json
Model - Response:
Message(
id = '9f1d0de8-75c2-40fa-b5e3-a80fcd292b8a', container = None, 
content = [TextBlock(citations = None,
                      text = 'It looks like there were already 3 tasks (IDs 1-3) from before. Here\'s the current setup:\n\n| Task ID | Subject | Depends On |\n|---------|---------|------------|\n| **4** | Setup project | *(none)* |\n| **5** | Write code | Task 4 |\n| **6** | Write tests | Task 5 |\n\nThe dependency chain is: **Setup project (4)** → **Write code (5)** → **Write tests (6)**\n\n- "Setup project" can start immediately.\n- "Write code" is blocked until "Setup project" is complete.\n- "Write tests" is blocked until "Write code" is complete.',
                       type = 'text')], 
model = 'deepseek-chat',
role = 'assistant',
stop_details = None,
stop_reason = 'end_turn',
stop_sequence = None, type = 'message', 
usage = Usage(cache_creation = None, cache_creation_input_tokens = 0, cache_read_input_tokens = 1408, inference_geo = None,
              input_tokens = 143, output_tokens = 142, server_tool_use = None, service_tier = 'standard'))
```

【stop_reason 不满足继续循环条件，跳出本次Agent循环，继续等待用户的下一次输入】
```json
It looks like there were already 3 tasks (IDs 1-3) from before. Here's the current setup:

| Task ID | Subject | Depends On |
|---------|---------|------------|
| **4** | Setup project | *(none)* |
| **5** | Write code | Task 4 |
| **6** | Write tests | Task 5 |

The dependency chain is: **Setup project (4)** → **Write code (5)** → **Write tests (6)**

- "Setup project" can start immediately.
- "Write code" is blocked until "Setup project" is complete.
- "Write tests" is blocked until "Write code" is complete.

```

