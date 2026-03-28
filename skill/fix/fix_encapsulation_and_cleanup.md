# Feature: 封装性和代码清理优化

## 1. 背景（是什么 && 为什么）

### 现状分析
代码中存在封装性和代码清理问题：
- `NodeWrapper` 和 `EdgeWrapper` 所有成员都是公共的
- 大量注释掉的代码未被清理
- 缺少移动语义优化
- 全局静态变量初始化顺序问题

### 设计问题
- **具体的技术问题**: 封装性差，代码冗余
- **架构层面的不足**: 缺少代码清理机制
- **用户体验的缺陷**: 代码可读性和可维护性差

## 2. 目标（想做成什么样子）

### 核心目标
- 改善 `NodeWrapper` 和 `EdgeWrapper` 的封装性
- 清理注释掉的代码
- 添加移动语义优化
- 改善全局静态变量的初始化顺序

### 预期效果
- **功能层面的改进**: 无功能变化
- **性能层面的提升**: 移动语义可提升性能
- **用户体验的优化**: 代码更清晰，更易维护

## 3. 范围明确（需要修改与新增那些文件,不能修改和新增那些文件）

### 需要修改的文件
- `framework/include/nndeploy/dag/util.h` - NodeWrapper 和 EdgeWrapper 封装
- 所有包含注释代码的文件 - 清理注释代码
- 返回值类型的函数 - 添加移动语义

### 需要新增的文件
- 无

### 不能修改的文件
- 公共 API（如果封装改变会影响调用方）- 需要谨慎处理

### 影响范围
- 整个代码库的代码质量

## 4. 设计方案（大致的方案）

### NodeWrapper 和 EdgeWrapper 封装改进

**旧代码（所有成员公共）:**
```cpp
class NNDEPLOY_CC_API NodeWrapper {
 public:
  bool is_external_;
  Node *node_;
  std::string name_;
  std::vector<NodeWrapper *> predecessors_;
  std::vector<NodeWrapper *> successors_;
  base::NodeColorType color_ = base::kNodeColorWhite;
};
```

**新代码（改进封装）:**
```cpp
class NNDEPLOY_CC_API NodeWrapper {
 public:
  NodeWrapper() : is_external_(false), node_(nullptr),
                  color_(base::kNodeColorWhite) {}

  // Getter 方法
  bool isExternal() const { return is_external_; }
  Node* node() const { return node_; }
  const std::string& name() const { return name_; }
  const std::vector<NodeWrapper*>& predecessors() const { return predecessors_; }
  const std::vector<NodeWrapper*>& successors() const { return successors_; }
  base::NodeColorType color() const { return color_; }

  // Setter 方法（选择性暴露）
  void setExternal(bool external) { is_external_ = external; }
  void setNode(Node* node) { node_ = node; }
  void setName(const std::string& name) { name_ = name; }
  void setColor(base::NodeColorType color) { color_ = color; }

  // 内部使用的修改方法
  void addPredecessor(NodeWrapper* pred) { predecessors_.push_back(pred); }
  void addSuccessor(NodeWrapper* succ) { successors_.push_back(succ); }

 private:
  bool is_external_;
  Node *node_;
  std::string name_;
  std::vector<NodeWrapper *> predecessors_;
  std::vector<NodeWrapper *> successors_;
  base::NodeColorType color_;
};
```

**注意**: 由于 `NodeWrapper` 可能被广泛使用，这种改变可能影响很多代码。建议采用渐进式方法：
1. 先添加 getter/setter 方法
2. 逐步迁移使用新方法
3. 最终将成员变为 private（可能需要大版本）

### 注释代码清理
- 搜索所有被注释的代码块
- 评估是否有保留价值
- 无价值的直接删除
- 有价值的迁移到文档或示例

### 移动语义优化
对于返回 `std::vector` 或 `std::string` 的函数：
```cpp
// 旧代码
std::vector<int> getItems() {
    std::vector<int> result;
    // ... 填充 result
    return result;  // 依赖于 RVO，可能触发拷贝
}

// 新代码（显式移动语义）
std::vector<int> getItems() {
    std::vector<int> result;
    // ... 填充 result
    return std::move(result);  // 显式移动
}

// 更好的方式（直接返回）
std::vector<int> getItems() {
    std::vector<int> result;
    // ... 填充 result
    return result;  // 现代编译器会自动优化（NRVO/RVO）
}
```

**注意**: 现代编译器（C++11+）会自动进行 RVO/NRVO 优化，显式 `std::move` 有时反而会阻止这种优化。需要根据具体情况评估。

### 全局静态变量初始化顺序
使用 `std::call_once` 确保单例模式的线程安全：
```cpp
std::map<base::EdgeType, std::shared_ptr<EdgeCreator>>&
getGlobalEdgeCreatorMap() {
    static std::once_flag once;
    static std::shared_ptr<std::map<...>> creators;
    std::call_once(once, []() {
        creators.reset(new std::map<...>);
    });
    return *creators;
}
```

## 5. 实施步骤

### Step 1: 清理注释代码
- 搜索所有被注释的代码
- 评估保留价值
- 删除无价值的注释代码
- 涉及文件: 整个代码库

### Step 2: 改进 NodeWrapper 和 EdgeWrapper（渐进式）
- 添加 getter/setter 方法
- 逐步迁移内部代码使用新方法
- 不立即将成员变为 private（避免破坏兼容）
- 涉及文件: `framework/include/nndeploy/dag/util.h`

### Step 3: 检查移动语义
- 识别返回大对象的函数
- 评估是否需要优化
- 编译器通常会自动优化，不需要显式 move
- 涉及文件: 整个代码库

### Step 4: 检查全局静态变量
- 检查所有使用静态变量的地方
- 确保使用了 `std::call_once` 或类似机制
- 涉及文件: 整个代码库

### Step 5: 代码审查
- 确认所有修改正确
- 验证没有破坏功能

### Step 6: 测试验证
- 运行所有测试
- 验证功能正常

### 兼容性与迁移
- **NodeWrapper/EdgeWrapper**: 渐进式改进，保持兼容
- **注释代码清理**: 直接删除，无兼容问题
- **移动语义**: 功能不变，只是优化
- **静态变量**: 改善线程安全，无兼容问题

## 6. 验收标准

### 功能测试
- **测试用例**: 确保所有修改后功能正常
  - 运行所有测试用例
  - 验证无功能变化

### 代码质量
- 代码通过编译
- 注释代码已清理
- 封装性改善
- 线程安全改善

### 回归测试
- 现有测试用例全部通过

### 性能测试
- 验证移动语义优化后的性能（如有明显变化）

### 文档与示例
- 更新开发文档

## 7. 其他说明

### 相关资源
- Code Review Report: P3 问题 #15, #22, #23, #26

### 风险与应对
- **潜在风险**: NodeWrapper 成员变为 private 可能破坏兼容性
  - 应对措施: 采用渐进式方法，先添加 getter/setter，再在大版本中改为 private
- **注意**: 此文档包含多个不同类型的改进，建议拆分为独立的 fix 文档
- **潜在风险**: 删除注释代码可能删除有用信息
  - 应对措施: 仔细评估，确认无价值再删除

### 依赖关系
- **依赖**: 无
- **被依赖**: 无

### 优先级
- 高优先级：注释代码清理
- 中优先级：静态变量初始化
- 低优先级：封装性改进（需要渐进式）
