# Feature: ConditionExecutor::process 数组越界检查修复

## 1. 背景（是什么 && 为什么）

### 现状分析
- `ConditionExecutor::process()` 中使用 `index_` 访问 `node_repository_`，但没有边界检查
- `index_` 可能越界，导致数组越界访问，引发崩溃
- 这是一个典型的不安全数组访问问题

### 设计问题
- **具体的技术问题**: 缺少数组边界检查
- **架构层面的不足**: 没有防御性编程实践
- **用户体验的缺陷**: 可能导致数组越界，引发崩溃

## 2. 目标（想做成什么样子）

### 核心目标
- 在 `ConditionExecutor::process()` 中添加数组边界检查
- 在越界时返回明确的错误状态
- 防止数组越界访问

### 预期效果
- **功能层面的改进**: 防止数组越界访问，提高代码健壮性
- **性能层面的提升**: 无明显性能影响
- **用户体验的优化**: 提供明确的错误信息，便于调试

## 3. 范围明确（需要修改与新增那些文件,不能修改和新增那些文件）

### 需要修改的文件
- `framework/source/nndeploy/dag/executor/condition_executor.cc` - 添加边界检查

### 需要新增的文件
- 无

### 不能修改的文件
- `framework/include/nndeploy/dag/executor/condition_executor.h` - 函数签名不需要改变
- 其他 executor 相关文件 - 不影响其他模块

### 影响范围
- 所有使用 ConditionExecutor 的场景
- 条件分支执行流程

## 4. 设计方案（大致的方案）

### 新旧方案对比
- **旧方案**: 直接使用 `index_` 访问数组，没有边界检查
- **新方案**: 在访问前检查 `index_` 是否在有效范围内
- **核心变化**: 添加防御性检查

### 架构/接口设计
- `process()` 函数签名保持不变
- 返回 `base::kStatusCodeErrorInvalidValue` 表示越界错误

### 核心操作流程
```
ConditionExecutor::process():
1. 检查 index_ 是否在有效范围内:
   if (index_ < 0 || index_ >= node_repository_.size()):
     记录错误日志
     返回 kStatusCodeErrorInvalidValue
2. 获取当前节点:
   Node *cur_node = node_repository_[index_]->node_
3. 检查节点是否有效:
   if (cur_node == nullptr):
     记录错误日志
     返回 kStatusCodeErrorNullParam
4. 运行节点:
   status = cur_node->run()
5. 更新 index_
6. 返回 status
```

### 技术细节
- 检查负数和超过数组大小的两种越界情况
- 返回明确的错误代码，便于调用方处理
- 记录详细的错误日志，包含 index_ 和数组大小的信息

## 5. 实施步骤

### Step 1: 在 ConditionExecutor::process 添加边界检查
- 修改 `framework/source/nndeploy/dag/executor/condition_executor.cc` 第 59-97 行
- 在访问 `node_repository_` 前添加边界检查
- 添加错误日志记录
- 涉及文件: `framework/source/nndeploy/dag/executor/condition_executor.cc`

### Step 2: 检查其他类似代码
- 检查 ConditionExecutor 的其他方法是否也有类似问题
- 检查其他 executor 是否有类似的数组访问
- 确保所有数组访问都有边界检查
- 涉及文件: `framework/source/nndeploy/dag/executor/`

### Step 3: 代码审查
- 检查所有数组访问点
- 确认没有遗漏
- 验证错误处理正确
- 涉及文件: `framework/source/nndeploy/dag/executor/condition_executor.cc`

### Step 4: 测试验证
- 测试正常场景
- 测试越界场景
- 测试空 node_repository_ 场景
- 测试负数 index 场景
- 涉及文件: `test/dag/executor/` (需要新增测试)

### 兼容性与迁移
- 向后兼容策略: 接口不变，只是添加安全检查
- 迁移路径: 无需迁移，直接增强安全性
- 过渡期安排: 无

## 6. 验收标准

### 功能测试
- **测试用例 1**: 正常场景
  - 创建 ConditionExecutor
  - 设置有效的 index_
  - 调用 process()
  - 验证正常执行
- **测试用例 2**: 越界场景（正数）
  - 创建 ConditionExecutor
  - 设置 index_ >= node_repository_.size()
  - 调用 process()
  - 验证返回 kStatusCodeErrorInvalidValue
  - 验证记录了错误日志
- **测试用例 3**: 越界场景（负数）
  - 创建 ConditionExecutor
  - 设置 index_ < 0
  - 调用 process()
  - 验证返回 kStatusCodeErrorInvalidValue
  - 验证记录了错误日志
- **测试用例 4**: 空 node_repository_ 场景
  - 创建 ConditionExecutor
  - 不添加任何节点
  - 调用 process()
  - 验证返回错误

### 代码质量
- 代码通过编译
- 符合现有代码规范
- 错误日志格式正确

### 回归测试
- 现有 DAG 测试用例全部通过
- 不影响其他 executor 相关功能
- 不影响 Graph 执行

### 性能与可维护性
- 边界检查的开销极小，可忽略
- 代码更安全，更健壮

### 文档与示例
- 更新相关文档说明错误处理
- 添加代码注释

## 7. 其他说明

### 相关资源
- Code Review Report: P1 问题 #8
- 相关文件: `framework/source/nndeploy/dag/executor/condition_executor.cc:59-97`

### 风险与应对
- **潜在风险**: 现有代码可能依赖 index_ 越界的"错误行为"
  - 应对措施: 这种依赖本身就是 bug，应该修复
- **潜在风险**: 其他地方也可能有类似问题
  - 应对措施: 代码审查时检查所有数组访问

### 依赖关系
- **依赖**: 无
- **被依赖**: 所有使用 ConditionExecutor 的场景

### 后续优化
- 在 select() 方法中添加类似的检查
- 考虑使用 at() 替代 [] 以获得自动边界检查（C++ STL）
- 添加断言检查，便于调试模式下的问题发现
