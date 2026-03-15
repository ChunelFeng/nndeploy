# Feature: 命名和拼写问题修正

## 1. 背景（是什么 && 为什么）

### 现状分析
代码中存在多处命名和拼写问题，影响代码可读性和专业性：
- 枚举值拼写错误：`NotSupport` 应为 `NotSupported`
- 注释拼写错误：`sopport` 应为 `support`
- 变量命名不一致：`creater_map` 应为 `creator_map`
- 函数命名拼写错误：`destory()` 应为 `destroy()`

### 设计问题
- **具体的技术问题**: 拼写错误影响可读性和专业性
- **架构层面的不足**: 缺少代码审查机制
- **用户体验的缺陷**: 代码不够专业，影响开发者体验

## 2. 目标（想做成什么样子）

### 核心目标
- 修正所有拼写错误
- 统一命名风格
- 提高代码可读性和专业性

### 预期效果
- **功能层面的改进**: 无功能变化
- **性能层面的提升**: 无性能影响
- **用户体验的优化**: 代码更规范，更易读

## 3. 范围明确（需要修改与新增那些文件,不能修改和新增那些文件）

### 需要修改的文件
- `framework/include/nndeploy/base/common.h` - 枚举值拼写修正
- `framework/source/nndeploy/inference.cc` - 变量名修正
- `framework/source/nndeploy/dag/executor/condition_executor.cc` - 变量名修正
- `framework/source/nndeploy/dag/edge/data_packet.cc` - 函数名修正

### 需要新增的文件
- 无

### 不能修改的文件
- 任何暴露在公共 API 的枚举值（可能破坏兼容性）- 需要谨慎处理

### 影响范围
- 所有使用这些枚举值、变量、函数的代码

## 4. 设计方案（大致的方案）

### 问题清单

#### 枚举值拼写错误（注意：可能破坏兼容性）
```cpp
// 旧值
kDataTypeCodeNotSupport
kDataFormatNotSupport
kPowerTypeNotSupport

// 新值（拼写正确）
kDataTypeCodeNotSupported
kDataFormatNotSupported
kPowerTypeNotSupported
```

#### 注释拼写错误
```cpp
// 旧注释
// this is not sopport

// 新注释
// this is not support
```

#### 变量命名不一致
```cpp
// 旧变量名
creater_map
safetensors_data_type
innner_position

// 新变量名
creator_map
safetensors_dtype  // 或简化为 dtype
inner_position
```

#### 函数命名拼写错误
```cpp
// 旧函数名
destory()

// 新函数名
destroy()
```

### 架构/接口设计
- 枚举值：需要考虑向后兼容，可能需要保留旧值并添加新值
- 函数名：如果是公共 API，需要保留旧函数并标记为 deprecated
- 变量名：内部变量可以直接修改，不破坏兼容性

## 5. 实施步骤

### Step 1: 修正注释拼写错误
- 修改 `framework/include/nndeploy/base/common.h` 中的注释
- 涉及文件: `framework/include/nndeploy/base/common.h`

### Step 2: 修正内部变量名
- 修改 `framework/source/nndeploy/inference.cc` 中的 `creater_map`
- 修改 `framework/source/nndeploy/dag/executor/condition_executor.cc` 中的 `innner_position`
- 涉及文件: 各相关源文件

### Step 3: 修正函数名（需谨慎）
- 修改 `framework/source/nndeploy/dag/edge/data_packet.cc` 中的 `destory()`
- 添加旧函数别名以保持兼容性
- 涉及文件: `framework/source/nndeploy/dag/edge/data_packet.cc`

### Step 4: 处理枚举值（需要大版本）
- 枚举值拼写错误由于可能破坏公共 API，建议在大版本中处理
- 可以先添加 `using` 别名或宏定义
- 涉及文件: `framework/include/nndeploy/base/common.h`

### Step 5: 搜索和更新所有引用
- 搜索所有使用旧名称的地方
- 更新为新名称
- 涉及文件: 整个代码库

### Step 6: 代码审查
- 确认所有修改正确
- 验证没有遗漏
- 确保编译通过

### Step 7: 测试验证
- 运行所有测试
- 验证功能正常

### 兼容性与迁移
- **枚举值**: 破坏性更改，需要在大版本中进行
- **函数名**: 可以保留旧函数并标记为 deprecated
- **变量名**: 内部变量，无兼容性问题

## 6. 验收标准

### 功能测试
- **测试用例**: 确保所有修改后功能正常
  - 运行所有测试用例
  - 验证无功能变化

### 代码质量
- 代码通过编译
- 拼写正确
- 命名一致
- 符合代码规范

### 回归测试
- 现有测试用例全部通过

### 文档与示例
- 更新文档中的旧名称

## 7. 其他说明

### 相关资源
- Code Review Report: P3 问题 #17, #18, #19

### 风险与应对
- **潜在风险**: 枚举值修改可能破坏公共 API
  - 应对措施: 在大版本中处理，或使用别名过渡
- **潜在风险**: 可能有外部代码依赖旧名称
  - 应对措施: 提供过渡期，保留旧名称一段时间

### 依赖关系
- **依赖**: 无
- **被依赖**: 无

### 优先级
- 高优先级：内部变量名、注释、函数名
- 低优先级：枚举值（需要大版本）
