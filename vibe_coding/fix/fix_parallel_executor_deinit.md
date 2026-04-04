---
name: fix_parallel_executor_deinit
title: ParallelTaskExecutor::deinit线程同步问题修复
description: 修复ParallelTaskExecutor::deinit中销毁线程池前未等待工作完成的线程同步问题
category: [fix]
difficulty: medium
priority: P1
status: planned
version: 1.0.0
tags: [concurrent, executor, thread_safety, lifecycle]
estimated_time: 4h
files_affected: [framework/source/nndeploy/dag/executor/parallel_task_executor.cc]
---

# Feature: ParallelTaskExecutor::deinit 线程同步问题修复

## 1. 背景（是什么 && 为什么）

### 现状分析
- `ParallelTaskExecutor::deinit()` 在销毁线程池前没有等待所有工作线程完成
- 线程池销毁后，工作线程可能还在运行，导致访问已销毁的节点
- 调用 `thread_pool_->destroy()` 后立即执行 `iter->node_->deinit()`，可能在线程还在使用时访问

### 设计问题
- **具体的技术问题**: 线程池销毁前缺少同步机制
- **架构层面的不足**: 生命周期管理不清晰，缺少优雅关闭机制
- **用户体验的缺陷**: 可能导致访问已销毁的资源，引发崩溃

## 2. 目标（想做成什么样子）

### 核心目标
- 在销毁线程池前等待所有工作线程完成
- 确保所有任务完成后再执行清理操作
- 提供优雅的关闭机制

### 预期效果
- **功能层面的改进**: ParallelTaskExecutor 能正确关闭，不会访问已销毁的资源
- **性能层面的提升**: 无性能影响，只是增加必要的等待
- **用户体验的优化**: 避免多线程场景下的崩溃

## 3. 范围明确（需要修改与新增那些文件,不能修改和新增那些文件）

### 需要修改的文件
- `framework/source/nndeploy/dag/executor/parallel_task_executor.cc` - 修复 `deinit()` 函数

### 需要新增的文件
- 无

### 不能修改的文件
- `framework/include/nndeploy/dag/executor/parallel_task_executor.h` - 函数签名不需要改变
- 其他 executor 相关文件 - 不影响其他模块

### 影响范围
- 所有使用 ParallelTaskExecutor 的场景
- 并行任务执行的生命周期管理

## 4. 设计方案（大致的方案）

### 新旧方案对比
- **旧方案**: 直接销毁线程池，然后 deinit 节点，没有等待工作完成
- **新方案**: 先请求终止所有 edges，然后同步等待所有工作完成，最后销毁线程池和 deinit 节点
- **核心变化**: 在销毁线程池前调用同步函数确保所有工作完成

### 架构/接口设计
- `deinit()` 函数签名保持不变
- 使用线程池提供的同步机制（如果有）

### 核心操作流程
```
ParallelTaskExecutor::deinit():
1. 请求终止所有 edge_repository_ 中的 edges:
   for edge in edge_repository_:
     edge.requestTerminate()
2. 等待所有工作线程完成:
   thread_pool_->synchronize()  // 确保所有任务完成
3. 销毁线程池:
   thread_pool_->destroy()
   delete thread_pool_
4. deinit 所有节点:
   for node in topo_sort_node_:
     node.deinit()
```

### 技术细节
- `requestTerminate()` 通知工作线程停止
- `synchronize()` 等待所有正在执行的任务完成
- 只有在所有工作完成后才销毁线程池
- 确保 node 的 deinit() 不会与工作线程的执行冲突

## 5. 实施步骤

### Step 1: 分析线程池 API
- 检查 ThreadPool 类是否提供 `synchronize()` 方法
- 如果没有，检查是否有其他同步机制
- 确定正确的同步调用方式
- 涉及文件: `framework/include/nndeploy/thread_pool/thread_pool.h`

### Step 2: 修复 ParallelTaskExecutor::deinit 函数
- 修改 `framework/source/nndeploy/dag/executor/parallel_task_executor.cc`
- 找到 `ParallelTaskExecutor::deinit()` 函数
- 在销毁线程池前添加同步调用
- 确保所有任务完成后再执行清理
- 涉及文件: `framework/source/nndeploy/dag/executor/parallel_task_executor.cc`

### Step 3: 检查其他 executor 的 deinit
- 检查 PipelineExecutor 的 deinit
- 检查 ConditionExecutor 的 deinit
- 确保其他 executor 没有类似问题
- 涉及文件: `framework/source/nndeploy/dag/executor/`

### Step 4: 代码审查
- 检查线程安全保证
- 确认没有竞态条件
- 验证生命周期管理正确
- 涉及文件: `framework/source/nndeploy/dag/executor/parallel_task_executor.cc`

### Step 5: 测试验证
- 测试正常关闭场景
- 测试有任务运行时的关闭
- 测试极端并发场景
- 使用 ThreadSanitizer 验证
- 涉及文件: `test/dag/executor/` (需要新增测试)

### 兼容性与迁移
- 向后兼容策略: 接口不变，内部实现修复
- 迁移路径: 无需迁移，直接修复 bug
- 过渡期安排: 无

## 6. 验收标准

### 功能测试
- **测试用例 1**: 正常关闭场景
  - 创建 ParallelTaskExecutor
  - 执行一些任务
  - 调用 deinit()
  - 验证正常关闭，无崩溃
- **测试用例 2**: 任务运行时关闭
  - 创建 ParallelTaskExecutor
  - 启动长时间运行的任务
  - 在任务运行时调用 deinit()
  - 验证任务正确终止，无崩溃
- **测试用例 3**: 多次 init/deinit
  - 多次调用 init() 和 deinit()
  - 验证每次都能正确工作
- **测试用例 4**: 极端并发场景
  - 创建多个 ParallelTaskExecutor
  - 大量并发任务
  - 同时关闭所有 executor
  - 验证线程安全

### 代码质量
- 代码通过编译
- ThreadSanitizer 检测无数据竞争
- 符合现有代码规范

### 回归测试
- 现有 DAG 测试用例全部通过
- 不影响其他 executor 相关功能
- 不影响 Graph 执行

### 性能与可维护性
- 关闭时间略微增加（等待任务完成），但在可接受范围内
- 代码逻辑更清晰，生命周期管理更正确

### 文档与示例
- 更新相关文档说明生命周期管理
- 添加代码注释

## 7. 其他说明

### 相关资源
- Code Review Report: P1 问题 #7
- 相关文件: `framework/source/nndeploy/dag/executor/parallel_task_executor.cc:43-59`
- 相关文件: `framework/include/nndeploy/thread_pool/thread_pool.h`

### 风险与应对
- **潜在风险**: 线程池可能没有提供 synchronize() 方法
  - 应对措施: 检查线程池实现，使用其他同步机制或添加同步方法
- **潜在风险**: 某些任务可能无法正确响应终止请求
  - 应对措施: 检查所有任务是否正确检查终止条件

### 依赖关系
- **依赖**: 无
- **被依赖**: 所有使用 ParallelTaskExecutor 的场景

### 后续优化
- 添加超时机制，防止无限等待
- 改进终止请求机制，使其更优雅
