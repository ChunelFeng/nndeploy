---
name: fix_priority
title: 修复优先级总览
description: 14个修复项的优先级排序、依赖关系和实施路线
category: [fix, plan]
status: planned
version: 1.0.0
tags: [priority, roadmap]
---

# 修复优先级总览

## P0 — 最高优先级（内存安全/崩溃/数据损坏）

这些问题可能在生产环境中导致崩溃、内存泄漏、双重释放，必须最先修复。

| 优先序 | 修复项 | 问题类型 | 风险 | 预估时间 |
|:---:|--------|----------|------|:---:|
| 1 | `fix_buffer_destructor` | Buffer 析构函数竞态条件 - 双重释放/内存泄漏 | 多线程下随时崩溃 | 4h |
| 2 | `fix_buffer_assignment` | Buffer `operator=` 引用计数错漏 - 内存泄漏 | 每次赋值都泄漏 | 4h |
| 3 | `fix_tensor_deallocate` | Tensor `deallocate()` 同样的竞态问题 - 双重释放 | 与 Buffer 同源 | 4h |
| 4 | `fix_pipeline_edge_destructor` | PipelineEdge 析构未通知等待线程 - 死锁 | Pipeline 模式必现 | 4h |

说明:
- #1 和 #2 一起修（同文件 `buffer.cc`，逻辑耦合）
- #3 参照 #1 的模式修复

## P1 — 高优先级（线程安全/崩溃风险）

这些问题在特定场景（并行执行、条件分支）下会触发崩溃。

| 优先序 | 修复项 | 问题类型 | 风险 | 预估时间 |
|:---:|--------|----------|------|:---:|
| 5 | `fix_parallel_executor_deinit` | 线程池销毁前未等待线程完成 - 访问已释放资源 | 并行任务退出时崩溃 | 4h |
| 6 | `fix_condition_executor_bounds` | 数组越界无检查 - 未定义行为/崩溃 | 条件分支时崩溃 | 2h |
| 7 | `fix_edge_gettypename` | 空指针解引用 - 崩溃 | 未初始化 Edge 时崩溃 | 2h |

## P2 — 中优先级（RAII/异常安全/API 安全）

这些问题不会在正常路径触发，但在异常路径或特定使用模式下可能产生问题。

| 优先序 | 修复项 | 问题类型 | 风险 | 预估时间 |
|:---:|--------|----------|------|:---:|
| 8 | `fix_tensor_copyto` | 裸指针管理临时对象 - 异常时内存泄漏 | 异常路径泄漏 | 2h |
| 9 | `fix_tensor_print` | host_buffer 管理混乱 - 可能误删原始 buffer | 调试时风险 | 2h |
| 10 | `fix_ring_queue_popfront` | `popFront()` 无法区分空/有效值 - 逻辑错误 | 语义歧义 | 2h |
| 11 | `fix_casting_and_safety` | C 风格转换 + dynamic_cast 未检查 | 潜在未定义行为 | 4h |

## P3 — 低优先级（代码质量/可维护性）

这些问题不影响正确性，但影响代码质量和可维护性。

| 优先序 | 修复项 | 问题类型 | 风险 | 预估时间 |
|:---:|--------|----------|------|:---:|
| 12 | `fix_maybe_destructor` | 不必要的虚析构函数 - vtable 开销 | 微小性能影响 | 2h |
| 13 | `fix_naming_and_spelling` | 拼写错误/命名不一致 | 可读性 | 2h |
| 14 | `fix_encapsulation_and_cleanup` | 封装性差 + 注释代码 + 移动语义 | 可维护性 | 4h |

## 推荐实施路线

```
第一轮 (P0, 约2天):
  fix_buffer_destructor + fix_buffer_assignment
    -> fix_tensor_deallocate
    -> fix_pipeline_edge_destructor

第二轮 (P1, 约1天):
  fix_parallel_executor_deinit
    -> fix_condition_executor_bounds
    -> fix_edge_gettypename

第三轮 (P2, 约1.5天):
  fix_tensor_copyto
    -> fix_tensor_print
    -> fix_ring_queue_popfront
    -> fix_casting_and_safety

第四轮 (P3, 约1天):
  fix_maybe_destructor
    -> fix_naming_and_spelling
    -> fix_encapsulation_and_cleanup
```

总预估时间: 约 5.5 天（42h）

## 关键依赖关系

- `fix_buffer_destructor` 应先于 `fix_tensor_deallocate`（后者参考前者的实现模式）
- `fix_buffer_assignment` 应与 `fix_buffer_destructor` 一起修复（同文件，逻辑耦合）
- `fix_naming_and_spelling` 中枚举值改名可能破坏公共 API，建议留到大版本发布时处理

## 排序依据

1. **影响严重性**: 崩溃/数据损坏 > 死锁 > 逻辑错误 > 代码质量
2. **触发概率**: 正常路径必现 > 多线程场景触发 > 异常路径触发 > 不影响运行
3. **修复耦合度**: 同文件/同模式的修复归为一组，降低回归风险
4. **兼容性风险**: 接口不变的修复优先，涉及公共 API 变更的延后
