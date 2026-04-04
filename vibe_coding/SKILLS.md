# nndeploy Skills 索引

## 概述

本索引文档汇集了 nndeploy 项目的所有技能（Skill）文档，按照功能分类组织，方便 AI Agent 和开发者快速查找和使用。

## 技能分类

### Feature（新功能）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| [feature_dynamic_graph](feature/feature_dynamic_graph.md) | 动态图修改，运行时添加/删除节点和边 | Hard | Planned | P2 |
| [feature_message_history](feature/feature_message_history.md) | 消息历史管理，支持对话上下文 | Medium | Planned | P2 |
| [feature_reducer](feature/feature_reducer.md) | 数据归约处理，支持多种聚合操作 | Medium | Planned | P2 |
| [feature_context_management](feature/feature_context_management.md) | 执行上下文管理，支持参数传递和调用追踪 | Medium | Planned | P2 |
| [feature_node_skill_tool_doc](feature/feature_node_skill_tool_doc.md) | 节点技能工具文档，增强节点能力 | Easy | Planned | P3 |
| [feature_edge_default_value](feature/feature_edge_default_value.md) | 边默认值支持，简化节点连接 | Easy | Planned | P3 |
| [feature_time_travel](feature/feature_time_travel.md) | 时间回溯机制，支持执行状态快照和恢复 | Hard | Planned | P2 |
| [feature_debugging_tools](feature/feature_debugging.md) | 调试工具集，支持断点和变量查看 | Medium | Planned | P2 |
| [feature_event_driven](feature/feature_event_driven.md) | 事件驱动架构，支持异步事件处理 | Hard | Planned | P2 |
| [feature_callback_system](feature/feature_callback_system.md) | 回调系统，支持节点间事件通信 | Medium | Planned | P2 |
| [feature_streaming](feature/feature_streaming.md) | 流式输出机制，支持实时数据推送 | Hard | Planned | P2 |
| [feature_human_feedback](feature/feature_human_feedback.md) | 人工反馈机制，支持人机协作 | Medium | Planned | P3 |
| [feature_checkpoint](feature/feature_checkpoint.md) | 检查点系统，支持状态持久化和恢复 | Medium | Planned | P2 |

### Fix（Bug 修复）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| [fix_buffer_assignment](fix/fix_buffer_assignment.md) | Buffer 赋值运算符引用计数问题修复 | Medium | Planned | P1 |
| [fix_tensor_deallocate](fix/fix_tensor_deallocate.md) | Tensor::deallocate 引用计数问题修复 | Medium | Planned | P1 |
| [fix_tensor_print](fix/fix_tensor_print.md) | Tensor 打印输出修复 | Easy | Planned | P2 |
| [fix_buffer_serialize](fix/fix_buffer_serialize.md) | Buffer 序列化问题修复 | Medium | Planned | P2 |
| [fix_ring_queue_popfront](fix/fix_ring_queue_popfront.md) | 环形队列 popFront 问题修复 | Easy | Planned | P2 |
| [fix_buffer_destructor](fix/fix_buffer_destructor.md) | Buffer 析构函数修复 | Medium | Planned | P1 |
| [fix_condition_executor_bounds](fix/fix_condition_executor_bounds.md) | 条件执行器边界检查修复 | Easy | Planned | P2 |
| [fix_naming_and_spelling](fix/fix_naming_and_spelling.md) | 命名和拼写问题修复 | Easy | Planned | P3 |
| [fix_encapsulation_and_cleanup](fix/fix_encapsulation_and_cleanup.md) | 封装和清理问题修复 | Medium | Planned | P2 |
| [fix_tensor_copyto](fix/fix_tensor_copyto.md) | Tensor copyTo 方法修复 | Medium | Planned | P2 |
| [fix_inference_canop](fix/fix_inference_canop.md) | 推理 CA 算子修复 | Hard | Planned | P1 |
| [fix_casting_and_safety](fix/fix_casting_and_safety.md) | 类型转换和安全性问题修复 | Medium | Planned | P1 |
| [fix_parallel_executor_deinit](fix/fix_parallel_executor_deinit.md) | 并行执行器清理修复 | Medium | Planned | P1 |
| [fix_pipeline_edge_destructor](fix/fix_pipeline_edge_destructor.md) | 流水线边析构函数修复 | Medium | Planned | P1 |
| [fix_edge_gettypename](fix/fix_edge_gettypename.md) | Edge 的 getTypeName 修复 | Easy | Planned | P3 |
| [fix_maybe_destructor](fix/fix_maybe_destructor.md) | Maybe 析构函数修复 | Medium | Planned | P1 |

### Build（构建系统）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| 待补充 | 待补充 | - | - | - |

### Deploy（部署运维）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| 待补充 | 待补充 | - | - | - |

### Test（测试验证）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| 待补充 | 待补充 | - | - | - |

### Plugin（插件开发）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| 待补充 | 待补充 | - | - | - |

### Workflow（工作流操作）

| 技能 | 描述 | 难度 | 状态 | 优先级 |
|------|------|------|------|--------|
| 待补充 | 待补充 | - | - | - |

## 按难度分类

### Easy（简单任务，1-2小时可完成）

**Feature:**
- [feature_node_skill_tool_doc](feature/feature_node_skill_tool_doc.md) - 节点技能工具文档
- [feature_edge_default_value](feature/feature_edge_default_value.md) - 边默认值支持
- [feature_human_feedback](feature/feature_human_feedback.md) - 人工反馈机制

**Fix:**
- [fix_tensor_print](fix/fix_tensor_print.md) - Tensor 打印输出
- [fix_ring_queue_popfront](fix/fix_ring_queue_popfront.md) - 环形队列 popFront
- [fix_condition_executor_bounds](fix/fix_condition_executor_bounds.md) - 条件执行器边界检查
- [fix_naming_and_spelling](fix/fix_naming_and_spelling.md) - 命名和拼写
- [fix_edge_gettypename](fix/fix_edge_gettypename.md) - Edge 的 getTypeName

### Medium（中等复杂度，半天可完成）

**Feature:**
- [feature_message_history](feature/feature_message_history.md) - 消息历史管理
- [feature_reducer](feature/feature_reducer.md) - 数据归约处理
- [feature_context_management](feature/feature_context_management.md) - 执行上下文管理
- [feature_debugging_tools](feature/feature_debugging.md) - 调试工具集
- [feature_callback_system](feature/feature_callback_system.md) - 回调系统
- [feature_checkpoint](feature/feature_checkpoint.md) - 检查点系统

**Fix:**
- [fix_buffer_assignment](fix/fix_buffer_assignment.md) - Buffer 赋值运算符
- [fix_tensor_deallocate](fix/fix_tensor_deallocate.md) - Tensor::deallocate
- [fix_buffer_serialize](fix/fix_buffer_serialize.md) - Buffer 序列化
- [fix_buffer_destructor](fix/fix_buffer_destructor.md) - Buffer 析构函数
- [fix_encapsulation_and_cleanup](fix/fix_encapsulation_and_cleanup.md) - 封装和清理
- [fix_tensor_copyto](fix/fix_tensor_copyto.md) - Tensor copyTo
- [fix_casting_and_safety](fix/fix_casting_and_safety.md) - 类型转换和安全性
- [fix_parallel_executor_deinit](fix/fix_parallel_executor_deinit.md) - 并行执行器清理
- [fix_pipeline_edge_destructor](fix/fix_pipeline_edge_destructor.md) - 流水线边析构
- [fix_maybe_destructor](fix/fix_maybe_destructor.md) - Maybe 析构函数

### Hard（复杂任务，需要1-2天）

**Feature:**
- [feature_dynamic_graph](feature/feature_dynamic_graph.md) - 动态图修改
- [feature_event_driven](feature/feature_event_driven.md) - 事件驱动架构
- [feature_streaming](feature/feature_streaming.md) - 流式输出
- [feature_time_travel](feature/feature_time_travel.md) - 时间回溯机制

**Fix:**
- [fix_inference_canop](fix/fix_inference_canop.md) - 推理 CA 算子

## 按优先级分类

### P0（紧急，阻塞功能使用）

无

### P1（重要，影响用户体验）

**Fix:**
- [fix_buffer_assignment](fix/fix_buffer_assignment.md) - Buffer 赋值运算符引用计数
- [fix_tensor_deallocate](fix/fix_tensor_deallocate.md) - Tensor::deallocate 引用计数
- [fix_inference_canop](fix/fix_inference_canop.md) - 推理 CA 算子
- [fix_casting_and_safety](fix/fix_casting_and_safety.md) - 类型转换和安全性
- [fix_parallel_executor_deinit](fix/fix_parallel_executor_deinit.md) - 并行执行器清理
- [fix_pipeline_edge_destructor](fix/fix_pipeline_edge_destructor.md) - 流水线边析构
- [fix_maybe_destructor](fix/fix_maybe_destructor.md) - Maybe 析构函数
- [fix_buffer_destructor](fix/fix_buffer_destructor.md) - Buffer 析构函数

### P2（一般，性能优化或改进）

**Feature:**
- [feature_dynamic_graph](feature/feature_dynamic_graph.md) - 动态图修改
- [feature_message_history](feature/feature_message_history.md) - 消息历史管理
- [feature_reducer](feature/feature_reducer.md) - 数据归约处理
- [feature_context_management](feature/feature_context_management.md) - 执行上下文管理
- [feature_debugging_tools](feature/feature_debugging.md) - 调试工具集
- [feature_event_driven](feature/feature_event_driven.md) - 事件驱动架构
- [feature_callback_system](feature/feature_callback_system.md) - 回调系统
- [feature_streaming](feature/feature_streaming.md) - 流式输出
- [feature_time_travel](feature/feature_time_travel.md) - 时间回溯
- [feature_checkpoint](feature/feature_checkpoint.md) - 检查点系统

**Fix:**
- [fix_tensor_print](fix/fix_tensor_print.md) - Tensor 打印输出
- [fix_buffer_serialize](fix/fix_buffer_serialize.md) - Buffer 序列化
- [fix_ring_queue_popfront](fix/fix_ring_queue_popfront.md) - 环形队列 popFront
- [fix_condition_executor_bounds](fix/fix_condition_executor_bounds.md) - 条件执行器边界
- [fix_encapsulation_and_cleanup](fix/fix_encapsulation_and_cleanup.md) - 封装和清理
- [fix_tensor_copyto](fix/fix_tensor_copyto.md) - Tensor copyTo

### P3（可选，锦上添花）

**Feature:**
- [feature_node_skill_tool_doc](feature/feature_node_skill_tool_doc.md) - 节点技能工具文档
- [feature_edge_default_value](feature/feature_edge_default_value.md) - 边默认值支持
- [feature_human_feedback](feature/feature_human_feedback.md) - 人工反馈机制

**Fix:**
- [fix_naming_and_spelling](fix/fix_naming_and_spelling.md) - 命名和拼写
- [fix_edge_gettypename](fix/fix_edge_gettypename.md) - Edge 的 getTypeName

## 技能依赖图

### Buffer/Tensor 内存管理系列

```
fix_buffer_destructor ─┐
                      ├─> fix_buffer_assignment ──> fix_tensor_deallocate
fix_tensor_print ──────┘
```

### DAG 系列功能

```
feature_context_management ──> feature_callback_system ──> feature_event_driven
                                    │                          │
                                    └─────────> feature_streaming ────────┘
```

### 状态管理系列

```
feature_checkpoint ──> feature_time_travel
        │
        └──> feature_message_history
```

### 并发执行系列

```
fix_parallel_executor_deinit ──> fix_pipeline_edge_destructor
```

## 快速导航

### 按模块查找

| 模块 | 相关技能 |
|------|----------|
| **Device** | fix_buffer_assignment, fix_tensor_deallocate, fix_tensor_print |
| **DAG/Graph** | feature_dynamic_graph, feature_context_management |
| **Executor** | fix_parallel_executor_deinit, fix_pipeline_edge_destructor, fix_condition_executor_bounds |
| **Event** | feature_event_driven, feature_callback_system |
| **Streaming** | feature_streaming |
| **LLM** | feature_message_history, feature_human_feedback |
| **Inference** | fix_inference_canop |

### 按类型查找

| 类型 | 相关技能 |
|------|----------|
| **内存管理** | fix_buffer_assignment, fix_tensor_deallocate, fix_buffer_destructor |
| **并发安全** | fix_parallel_executor_deinit, fix_pipeline_edge_destructor, fix_casting_and_safety |
| **动态能力** | feature_dynamic_graph, feature_time_travel |
| **状态管理** | feature_checkpoint, feature_context_management, feature_message_history |
| **事件系统** | feature_event_driven, feature_callback_system |
| **数据传输** | feature_streaming, feature_reducer |

## 统计信息

- **总技能数**: 29
  - Feature: 13
  - Fix: 16
  - Build: 0
  - Deploy: 0
  - Test: 0
  - Plugin: 0
  - Workflow: 0
- **按难度**:
  - Easy: 9
  - Medium: 16
  - Hard: 5
- **按优先级**:
  - P0: 0
  - P1: 8
  - P2: 16
  - P3: 5

## 更新日志

| 日期 | 变更 |
|------|------|
| 2026-03-28 | 创建技能索引文档，添加现有29个技能 |
