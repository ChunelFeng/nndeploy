# Fix 开发路线图

本文档定义了所有 bug 修复和代码优化的开发顺序、优先级和依赖关系。

## 排序依据

1. **严重性优先**: P0（致命） > P1（严重） > P2（一般） > P3（优化）
2. **依赖关系**: 确保前置 fix 先完成
3. **风险控制**: 先修复独立且影响范围小的问题
4. **稳定性**: 确保核心组件（Buffer、Tensor）的线程安全和内存管理

---

## 阶段 1: P0 严重问题修复（1-2天）

### 1. fix_buffer_serialize
- **序号**: 1
- **优先级**: P0
- **依赖**: 无
- **预估工作量**: 1小时
- **风险**: 低
- **文件**: `framework/source/nndeploy/device/buffer.cc:212-242`
- **问题**: `serialize()` 函数返回未初始化的变量
- **排序理由**: 简单的返回值错误，独立修复，无副作用

### 2. fix_inference_canop
- **序号**: 2
- **优先级**: P0
- **依赖**: 无
- **预估工作量**: 1小时
- **风险**: 中（可能暴露被掩盖的 bug）
- **文件**: `framework/source/nndeploy/inference.cc:109-138`
- **问题**: `canOpInput()` 和 `canOpOutput()` 永远返回 false
- **排序理由**: 逻辑错误简单修复，但需要全面测试影响范围

### 3. fix_buffer_assignment
- **序号**: 3
- **优先级**: P0
- **依赖**: 无
- **预估工作量**: 2小时
- **风险**: 中（可能暴露内存泄漏）
- **文件**: `framework/source/nndeploy/device/buffer.cc:111-125`
- **问题**: 赋值运算符未处理原有资源的引用计数
- **排序理由**: Buffer 是基础组件，修复影响范围大，必须尽早处理

### 4. fix_buffer_destructor
- **序号**: 4
- **优先级**: P0
- **依赖**: fix_buffer_assignment
- **预估工作量**: 2小时
- **风险**: 中（多线程安全）
- **文件**: `framework/source/nndeploy/device/buffer.cc:153-167`
- **问题**: 析构函数线程安全问题
- **排序理由**: Buffer 线程安全是基础，需要在 Tensor 修复前完成

---

## 阶段 2: P1 内存管理和线程安全（2-3天）

### 5. fix_tensor_deallocate
- **序号**: 5
- **优先级**: P1
- **依赖**: fix_buffer_destructor
- **预估工作量**: 2小时
- **风险**: 中
- **文件**: `framework/source/nndeploy/device/tensor.cc:245-254`
- **问题**: Tensor 释放的引用计数问题
- **排序理由**: 依赖 Buffer 的修复方案作为参考

### 6. fix_parallel_executor_deinit
- **序号**: 6
- **优先级**: P1
- **依赖**: 无
- **预估工作量**: 3小时
- **风险**: 高（多线程同步）
- **文件**: `framework/source/nndeploy/dag/executor/parallel_task_executor.cc`
- **问题**: 线程池销毁前未等待工作完成
- **排序理由**: 可能导致访问已销毁资源，需要谨慎处理

### 7. fix_pipeline_edge_destructor
- **序号**: 7
- **优先级**: P1
- **依赖**: 无
- **预估工作量**: 2小时
- **风险**: 高（可能导致死锁）
- **文件**: `framework/source/nndeploy/dag/edge/pipeline_edge.cc`
- **问题**: 析构时未通知等待线程
- **排序理由**: 死锁风险高，需要优先修复

### 8. fix_condition_executor_bounds
- **序号**: 8
- **优先级**: P1
- **依赖**: 无
- **预估工作量**: 1小时
- **风险**: 中
- **文件**: `framework/source/nndeploy/dag/executor/condition_executor.cc:59-97`
- **问题**: 数组越界访问
- **排序理由**: 防御性检查，降低崩溃风险

---

## 阶段 3: P2 资源管理和 API 安全（1-2天）

### 9. fix_tensor_copyto
- **序号**: 9
- **优先级**: P2
- **依赖**: 无
- **预估工作量**: 1小时
- **风险**: 低
- **文件**: `framework/source/nndeploy/device/tensor.cc:383-424`
- **问题**: 临时对象未使用 RAII 管理
- **排序理由**: 代码质量改进，提高异常安全性

### 10. fix_tensor_print
- **序号**: 10
- **优先级**: P2
- **依赖**: 无
- **预估工作量**: 1小时
- **风险**: 低
- **文件**: `framework/source/nndeploy/device/tensor.cc:783-850`
- **问题**: host_buffer 管理不一致
- **排序理由**: 避免意外释放原始 buffer

### 11. fix_ring_queue_popfront
- **序号**: 11
- **优先级**: P2
- **依赖**: 无
- **预估工作量**: 2小时
- **风险**: 中（API 变更）
- **文件**: `framework/include/nndeploy/base/ring_queue.h:44-53`
- **问题**: 无法区分空队列和有效值
- **排序理由**: 需要添加新 API，保持向后兼容

### 12. fix_edge_gettypename
- **序号**: 12
- **优先级**: P2
- **依赖**: 无
- **预估工作量**: 0.5小时
- **风险**: 低
- **文件**: `framework/source/nndeploy/dag/edge.cc:329`
- **问题**: 空指针解引用
- **排序理由**: 简单的防御性检查

---

## 阶段 4: P3 代码质量和可维护性（1-2天）

### 13. fix_maybe_destructor
- **序号**: 13
- **优先级**: P3
- **依赖**: 无
- **预估工作量**: 0.5小时
- **风险**: 低
- **文件**: `framework/include/nndeploy/base/status.h:130`
- **问题**: 不必要的虚析构函数
- **排序理由**: 轻微性能优化，需要确认公共 API 兼容性

### 14. fix_naming_and_spelling
- **序号**: 14
- **优先级**: P3
- **依赖**: 无
- **预估工作量**: 1小时
- **风险**: 中（枚举值可能破坏兼容性）
- **文件**: 多个文件
- **问题**: 拼写错误和命名不一致
- **排序理由**: 内部变量可以直接修改，枚举值需要大版本处理

### 15. fix_casting_and_safety
- **序号**: 15
- **优先级**: P3
- **依赖**: 无
- **预估工作量**: 3小时
- **风险**: 低
- **文件**: 整个代码库（主要在 dag 模块）
- **问题**: C 风格转换和未检查的 dynamic_cast
- **排序理由**: 代码风格改进，需要全局搜索确认

### 16. fix_encapsulation_and_cleanup
- **序号**: 16
- **优先级**: P3
- **依赖**: 无
- **预估工作量**: 4小时
- **风险**: 中（可能影响兼容性）
- **文件**: 多个文件
- **问题**: 封装性差、注释代码未清理
- **排序理由**: 建议拆分为独立的 fix 文档

---

## 依赖关系图

```
P0 阶段:
  fix_buffer_serialize ─────────────────────────────┐
  fix_inference_canop ─────────────────────────────┤
  fix_buffer_assignment ────────────────────────────┤
  fix_buffer_destructor (依赖: fix_buffer_assignment) ──┤

P1 阶段:
  fix_tensor_deallocate (依赖: fix_buffer_destructor) ───┤
  fix_parallel_executor_deinit ─────────────────────────┤
  fix_pipeline_edge_destructor ─────────────────────────┤
  fix_condition_executor_bounds ─────────────────────────┤

P2 阶段:
  fix_tensor_copyto ─────────────────────────────────────┤
  fix_tensor_print ──────────────────────────────────────┤
  fix_ring_queue_popfront ───────────────────────────────┤
  fix_edge_gettypename ──────────────────────────────────┤

P3 阶段:
  fix_maybe_destructor ───────────────────────────────────┤
  fix_naming_and_spelling ───────────────────────────────┤
  fix_casting_and_safety ─────────────────────────────────┤
  fix_encapsulation_and_cleanup ───────────────────────────┤
```

---

## 关键里程碑

| 里程碑 | 完成条件 | 预计时间 |
|--------|----------|----------|
| M1: P0 完成 | 所有 P0 fix 完成，测试通过 | 1-2天 |
| M2: 核心组件稳定 | P0+P1 完成，Buffer/Tensor 线程安全 | 3-5天 |
| M3: 代码质量提升 | P0+P1+P2 完成，API 安全改进 | 4-7天 |
| M4: 全部完成 | 所有 fix 完成，代码质量达标 | 5-9天 |

---

## 高风险项标注

### 极高风险（需要特别关注）

1. **fix_parallel_executor_deinit**: 线程同步问题，容易引发死锁或崩溃
   - **应对**: 使用 ThreadSanitizer 验证
   - **测试**: 需要极端并发测试场景

2. **fix_pipeline_edge_destructor**: 条件变量通知问题，可能导致死锁
   - **应对**: 确保所有等待路径都能被正确唤醒
   - **测试**: 需要创建多个等待线程的场景

3. **fix_buffer_destructor / fix_tensor_deallocate**: 原子操作和引用计数
   - **应对**: 使用 ASan/Valgrind 和 ThreadSanitizer 验证
   - **测试**: 需要多线程并发析构测试

### 中等风险

1. **fix_inference_canop**: 可能暴露被掩盖的 bug
   - **应对**: 全面测试所有推理场景

2. **fix_naming_and_spelling**: 枚举值修改可能破坏兼容性
   - **应对**: 建议在大版本中处理，或使用别名过渡

3. **fix_ring_queue_popfront**: API 变更
   - **应对**: 保持向后兼容，逐步迁移

---

## 测试要求

### 必需测试工具
- **ThreadSanitizer (TSan)**: 检测数据竞争
- **AddressSanitizer (ASan)**: 检测内存错误
- **Valgrind**: 内存泄漏检测

### 测试场景清单
1. 多线程并发析构（Buffer、Tensor）
2. 线程池关闭时的任务执行
3. 条件变量等待时的析构
4. 跨设备拷贝和序列化
5. 数组越界边界条件

---

## 注意事项

1. **编码风格**: 所有修改应符合现有代码规范
2. **注释**: 关键修改处添加注释说明原因
3. **兼容性**: 公共 API 变更需要考虑向后兼容
4. **测试**: 每个 fix 完成后必须运行相关测试
5. **Code Review**: 完成后进行 Code Review，确保没有遗漏问题
