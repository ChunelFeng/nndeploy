# nndeploy DAG 模块功能增强方案

## 背景

nndeploy 是一个 AI 部署框架，其 DAG 模块提供有向无环图的编排能力。通过对比 LangGraph 的功能特性，发现 nndeploy DAG 已具备基础的图执行能力，但在状态管理、持久化、回调等高级特性方面存在差距。

## 当前 nndeploy DAG 已实现的功能

| 功能 | 状态 | 说明 |
|------|------|------|
| 基本图结构 | ✅ | Graph、Node、Edge 基础类 |
| 串行执行 | ✅ | SequentialExecutor |
| 任务并行 | ✅ | ParallelTaskExecutor |
| 流水线并行 | ✅ | ParallelPipelineExecutor |
| 条件分支 | ✅ | ConditionExecutor + Condition 类 |
| 循环支持 | ✅ | Loop 基类 + FixedLoop |
| 子图嵌套 | ✅ | Graph 继承自 Node |
| 序列化/反序列化 | ✅ | 完整的 JSON 序列化 |
| 中断支持 | ✅ | Node/Executor interrupt 接口 |
| 拓扑排序 | ✅ | BFS/DFS 拓扑排序 |
| 设备间数据传输 | ✅ | 通过 Edge 实现 |

## nndeploy 当前中断机制分析

### 现有实现
```cpp
// Node 基类使用原子变量管理中断状态
std::atomic<bool> stop_;
bool interrupt() { stop_.exchange(true); }
bool checkInterruptStatus() { return stop_.load(); }
void clearInterrupt() { stop_.store(false); }
```

### 当前局限性
| 限制 | 说明 |
|------|------|
| 单向终止 | 只能终止执行，无法等待外部输入 |
| 无恢复机制 | 中断后无法继续执行，只能重新开始 |
| 无优先级 | 所有中断请求平等处理 |
| 推理层不可中断 | Inference 层不支持中途中断 |
| 同步限制 | 中断操作基本是同步的 |
| 状态不一致 | 中断后节点状态可能不完整 |

---

## 需要增强的功能（按优先级分类）

### 一、高优先级增强（核心缺失功能）

#### 1. 人类反馈能力（Human-in-the-Loop）🔴 重点
**问题描述：** 当前只有终止式中断，无法暂停等待人类输入后继续执行。

**LangGraph 参考：**
- `interrupt()` 节点暂停执行，保存状态
- 等待人类输入（确认/修改/跳过）
- 接收反馈后继续执行
- 与 Checkpoint 配合，支持断点续执行
- 支持多种反馈类型（Approval、Correction、Skip、Reroute）

**增强方案：**

##### 1.1 状态扩展
```cpp
// 扩展 Node 的状态机
enum NodeState {
    kNodeStateIdle,          // 空闲
    kNodeStateRunning,       // 运行中
    kNodeStateWaiting,       // ⭐ 新增：等待人类反馈
    kNodeStatePaused,       // ⭐ 新增：已暂停
    kNodeStateCompleted,     // 完成
    kNodeStateInterrupted,   // 被终止
    kNodeStateError          // 错误
};

// 新增人类反馈类型
enum HumanFeedbackType {
    kFeedbackApproval,       // 批准，继续执行
    kFeedbackCorrection,     // 修正，带修改内容
    kFeedbackSkip,           // 跳过当前节点
    kFeedbackReroute,        // 重新路由到其他节点
    kFeedbackTerminate       // 终止整个流程
};

// 人类反馈数据结构
struct HumanFeedback {
    HumanFeedbackType type;          // 反馈类型
    std::string node_name;            // 关联节点
    std::string user_id;              // 用户标识
    nlohmann::json data;              // 反馈数据（如修改内容）
    int64_t timestamp;                // 时间戳
    std::string session_id;           // 会话 ID
};
```

##### 1.2 核心接口设计
```cpp
// Node 基类新增接口
class Node {
public:
    // 请求人类反馈（同步）
    virtual bool requestHumanFeedback(const std::string& prompt,
                                      const nlohmann::json& context,
                                      int timeout_ms = -1);

    // 请求人类反馈（异步）
    virtual bool requestHumanFeedbackAsync(const std::string& prompt,
                                           const nlohmann::json& context,
                                           std::function<void(const HumanFeedback&)> callback);

    // 接收人类反馈
    virtual void onHumanFeedback(const HumanFeedback& feedback);

    // 检查是否在等待反馈
    virtual bool isWaitingForFeedback() const;

    // 获取等待的反馈信息
    virtual HumanFeedbackRequest* getFeedbackRequest() const;
};

// 新增：反馈请求结构
struct HumanFeedbackRequest {
    std::string request_id;           // 请求 ID
    std::string node_name;            // 请求节点
    std::string prompt;                // 提示语
    nlohmann::json context;            // 上下文数据
    nlohmann::json current_data;       // 当前节点数据
    std::vector<std::string> options;  // 可选操作
    int64_t created_at;               // 创建时间
    int timeout_ms;                   // 超时时间
};
```

##### 1.3 反馈管理器
```cpp
// 新增：人类反馈管理器
class HumanFeedbackManager {
public:
    // 提交反馈请求
    std::string submitRequest(const HumanFeedbackRequest& request);

    // 提供反馈（外部调用）
    bool provideFeedback(const std::string& request_id,
                         const HumanFeedback& feedback);

    // 获取等待中的请求列表
    std::vector<HumanFeedbackRequest> getPendingRequests(const std::string& session_id);

    // 设置反馈回调
    void setFeedbackHandler(std::function<void(const HumanFeedback&)> handler);

    // 超时处理
    void handleTimeout(const std::string& request_id);

private:
    std::map<std::string, HumanFeedbackRequest> pending_requests_;
    std::map<std::string, std::promise<HumanFeedback>> feedback_promises_;
    std::mutex requests_mutex_;
};
```

##### 1.4 反馈节点
```cpp
// 新增：专门的反馈节点
class HumanReviewNode : public Node {
public:
    HumanReviewNode(const std::string& name = "human_review");

    // 配置审核选项
    void setReviewOptions(const std::vector<std::string>& options);

    // 设置提示语模板
    void setPromptTemplate(const std::string& template_str);

    // 执行：触发人类审核
    virtual std::vector<Edge*> forward(std::vector<Edge*> inputs) override;

private:
    std::vector<std::string> review_options_;
    std::string prompt_template_;
    HumanFeedbackManager* feedback_manager_;
};
```

##### 1.5 与 Executor 集成
```cpp
// Executor 基类扩展
class Executor {
public:
    // 设置反馈管理器
    void setFeedbackManager(HumanFeedbackManager* manager);

    // 处理反馈后的节点恢复
    void resumeNode(Node* node, const HumanFeedback& feedback);

protected:
    HumanFeedbackManager* feedback_manager_ = nullptr;
};

// 恢复执行逻辑
void SequentialExecutor::resumeNode(Node* node, const HumanFeedback& feedback) {
    switch (feedback.type) {
        case kFeedbackApproval:
            // 继续正常执行
            node->clearInterrupt();
            continueExecution(node);
            break;
        case kFeedbackCorrection:
            // 应用修正数据，然后继续
            applyCorrection(node, feedback.data);
            node->clearInterrupt();
            continueExecution(node);
            break;
        case kFeedbackSkip:
            // 跳过当前节点，执行下一个
            skipNode(node);
            break;
        case kFeedbackReroute:
            // 重新路由到指定节点
            reroute(node, feedback.data["target_node"]);
            break;
        case kFeedbackTerminate:
            // 终止整个流程
            interrupt();
            break;
    }
}
```

##### 1.6 与 Checkpoint 集成
```cpp
// 扩展 Checkpoint 机制
class Checkpoint {
public:
    // 保存反馈状态
    void saveFeedbackRequest(const HumanFeedbackRequest& request);

    // 恢复时加载待处理反馈
    HumanFeedbackRequest* getPendingFeedback();

private:
    HumanFeedbackRequest pending_feedback_;
};

// GraphRunner 集成
void GraphRunner::runWithHumanReview() {
    while (hasPendingNodes()) {
        Node* node = getNextNode();
        node->run();

        // 检查是否等待反馈
        if (node->isWaitingForFeedback()) {
            // 保存检查点
            checkpoint_->save(graph_->getCurrentState());
            checkpoint_->saveFeedbackRequest(node->getFeedbackRequest());

            // 暂停等待反馈
            waitForFeedback();

            // 恢复执行
            HumanFeedback feedback = getFeedback();
            executor_->resumeNode(node, feedback);
        }
    }
}
```

##### 1.7 API 设计
```cpp
// DAG 公共 API
namespace nndeploy {
namespace dag {

// 提交人类反馈
API_EXPORT bool submitHumanFeedback(const std::string& request_id,
                                      const HumanFeedback& feedback);

// 获取待审核列表
API_EXPORT std::vector<HumanFeedbackRequest> getPendingReviews(const std::string& session_id);

// 设置全局反馈处理器
API_EXPORT void setGlobalFeedbackHandler(std::function<void(const HumanFeedback&)> handler);

} // namespace dag
} // namespace nndeploy
```

##### 1.8 使用示例
```cpp
// 场景1：图中的审核节点
HumanReviewNode* review = new HumanReviewNode("content_review");
review->setReviewOptions({
    "approve",      // 批准
    "reject",       // 拒绝
    "modify",       // 修改
    "skip"          // 跳过
});
review->setPromptTemplate("请审核以下内容: {content}");
graph->addNode(review);

// 场景2：运行时动态请求反馈
void CustomNode::forward(std::vector<Edge*> inputs) {
    // 执行业务逻辑
    auto result = process(inputs);

    // 检测需要人工审核
    if (needReview(result)) {
        requestHumanFeedback(
            "检测到可疑内容，请确认是否通过",
            {{"data", result}},
            30000  // 30秒超时
        );
    }
}

// 场景3：外部提供反馈
// 服务器端接口
void handleFeedbackRequest(const std::string& request_id,
                           const std::string& feedback_type,
                           const nlohmann::json& data) {
    HumanFeedback feedback;
    feedback.type = parseFeedbackType(feedback_type);
    feedback.data = data;
    feedback.timestamp = getCurrentTimestamp();

    nndeploy::dag::submitHumanFeedback(request_id, feedback);
}
```

**影响文件：**
```
新建：
├── framework/include/nndeploy/dag/human_feedback.h
└── framework/source/nndeploy/dag/human_feedback.cc

修改：
├── framework/include/nndeploy/dag/node.h          // 扩展状态和接口
├── framework/source/nndeploy/dag/node.cc
├── framework/include/nndeploy/dag/executor.h      // 集成反馈恢复
├── framework/source/nndeploy/dag/executor.cc
├── framework/include/nndeploy/dag/graph_runner.h  // 集成流程控制
└── framework/include/nndeploy/dag/checkpoint.h     // 保存反馈状态
```

---

#### 2. 状态持久化（Checkpoint）机制
**问题描述：** 当前 nndeploy 没有执行状态的持久化能力，无法保存和恢复图执行状态。

**LangGraph 参考：**
- 自动保存执行状态到检查点
- 支持 Redis、内存等多种存储后端
- 断点续执行能力
- 状态版本追踪

**增强方案：**
- 新增 `Checkpoint` 基类
- 实现 `MemoryCheckpoint`（内存存储）
- 实现 `RedisCheckpoint`（Redis 存储）
- 在 GraphRunner 中集成检查点保存/恢复
- 支持按节点执行前后保存状态
- 定义可序列化的状态格式

**影响文件：**
- 新建：`framework/include/nndeploy/dag/checkpoint.h`
- 新建：`framework/source/nndeploy/dag/checkpoint.cc`
- 修改：`framework/include/nndeploy/dag/graph.h`
- 修改：`framework/include/nndeploy/dag/graph_runner.h`

---

#### 3. 完整回调系统
**问题描述：** 当前只有回调函数类型定义，没有完整的生命周期回调管理系统。

**LangGraph 参考：**
- `on_start`: 节点执行前
- `on_end`: 节点执行后
- `on_error`: 发生错误时
- 支持回调链管理

**增强方案：**
- 新增 `CallbackHandler` 基类
- 定义生命周期钩子接口
- 在 Executor 中集成回调触发逻辑
- 支持多个回调处理器注册
- 支持异步回调执行

**影响文件：**
- 新建：`framework/include/nndeploy/dag/callback.h`
- 新建：`framework/source/nndeploy/dag/callback.cc`
- 修改：`framework/include/nndeploy/dag/executor.h`

---

#### 4. 流式输出（Streaming）机制
**问题描述：** 缺少实时数据推送能力，无法支持前端实时展示进度。

**LangGraph 参考：**
- Token 级别流式输出
- 消息级别流式输出
- WebSocket 集成支持
- 增量处理

**增强方案：**
- 新增 `StreamHandler` 接口
- 定义流式数据格式
- 在 Node 执行过程中推送中间结果
- 支持事件流（Event Stream）模式
- 可选的 WebSocket 集成

**影响文件：**
- 新建：`framework/include/nndeploy/dag/stream.h`
- 新建：`framework/source/nndeploy/dag/stream.cc`
- 修改：`framework/include/nndeploy/dag/node.h`
- 修改：`framework/include/nndeploy/dag/executor.h`

---

### 二、中优先级增强（体验优化）

#### 5. 状态更新策略（Reducer 模式）
**问题描述：** 当前通过 Edge 传输数据，缺少灵活的状态更新策略。

**LangGraph 参考：**
- `overwrite`: 直接覆盖（默认）
- `append_list`: 列表追加
- `add_messages`: 消息列表追加
- 自定义 reducer 函数

**增强方案：**
- 在 Edge 中支持多种更新策略
- 预定义常用 reducer 类型
- 支持自定义 reducer 函数
- 状态冲突解决机制

**影响文件：**
- 修改：`framework/include/nndeploy/dag/edge.h`

---

#### 6. 消息历史管理
**问题描述：** 没有对话上下文管理能力，难以支持多轮对话场景。

**LangGraph 参考：**
- 自动维护对话历史
- 上下文窗口管理
- 短期/长期记忆机制
- 会话管理

**增强方案：**
- 新增 `MessageHistory` 类
- 定义消息格式（role、content、metadata）
- 支持消息追加、截断、检索
- 与 Checkpoint 集成持久化

**影响文件：**
- 新建：`framework/include/nndeploy/dag/message.h`
- 新建：`framework/source/nndeploy/dag/message.cc`

---

#### 7. 时间旅行（Time Travel）
**问题描述：** 无法追溯和修改执行路径，调试能力有限。

**LangGraph 参考：**
- 保存完整的执行历史
- 支持回退到任意检查点
- 修改后继续执行

**增强方案：**
- 扩展 Checkpoint 机制，保存完整历史
- 新增 `TimeTravel` 接口
- 支持回退和分支执行
- 版本化管理状态变更

**影响文件：**
- 修改：`framework/include/nndeploy/dag/checkpoint.h`
- 修改：`framework/include/nndeploy/dag/graph_runner.h`

---

### 三、低优先级增强（扩展特性）

#### 8. 执行上下文（Context）管理
**问题描述：** 缺少统一的执行上下文管理。

**LangGraph 参考：**
- 当前执行位置追踪
- 调用链路管理
- 运行时配置传递

**增强方案：**
- 新增 `Context` 类
- 跟踪执行栈
- 传递运行时参数
- 线程安全的上下文访问

**影响文件：**
- 新建：`framework/include/nndeploy/dag/context.h`
- 新建：`framework/source/nndeploy/dag/context.cc`
- 修改：各 Executor 类

---

#### 9. 事件驱动执行
**问题描述：** 当前是轮询/命令式执行，不支持事件驱动模式。

**LangGraph 参考：**
- 基于事件的执行触发
- 支持外部事件输入
- 异步事件处理

**增强方案：**
- 新增 `Event` 类
- 实现 `EventDrivenExecutor`
- 事件订阅/发布机制
- 与现有 Executor 兼容

**影响文件：**
- 新建：`framework/include/nndeploy/dag/event.h`
- 新建：`framework/source/nndeploy/dag/event.cc`
- 新建：`framework/include/nndeploy/dag/event_executor.h`

---

#### 10. 动态图修改
**问题描述：** 图结构在运行时不可修改。

**LangGraph 参考：**
- 运行时添加/删除节点
- 动态修改边连接
- 热更新能力

**增强方案：**
- Graph 添加动态修改 API
- 线程安全的图结构变更
- 修改后的重新编译
- 回滚机制

**影响文件：**
- 修改：`framework/include/nndeploy/dag/graph.h`

---

#### 11. 调试和追踪工具
**问题描述：** 缺少可视化的调试和追踪能力。

**LangGraph 参考：**
- LangSmith 集成
- 可视化执行流程
- 性能分析工具
- 错误追踪

**增强方案：**
- 扩展调试模式输出
- 生成执行追踪日志
- 支持导出执行树
- 性能统计数据收集

**影响文件：**
- 修改：`framework/include/nndeploy/dag/graph.h`
- 修改：`framework/include/nndeploy/dag/graph_runner.h`

---

## 功能对比总结表

| 功能特性 | LangGraph | nndeploy DAG | 优先级 |
|----------|-----------|--------------|--------|
| 基础图结构 | ✅ | ✅ | - |
| 串行执行 | ✅ | ✅ | - |
| 任务并行 | ✅ | ✅ | - |
| 流水线并行 | ✅ | ✅ | - |
| 条件分支 | ✅ | ✅ | - |
| 循环 | ✅ | ✅ | - |
| 子图 | ✅ | ✅ | - |
| 序列化 | ✅ | ✅ | - |
| 终止式中断 | ✅ | ✅ | - |
| **人类反馈** | ✅ | ❌ | **🔴 最高** |
| Checkpoint | ✅ | ❌ | 高 |
| 完整回调 | ✅ | ⚠️ 部分 | 高 |
| 流式输出 | ✅ | ❌ | 高 |
| Reducer | ✅ | ❌ | 中 |
| 消息历史 | ✅ | ❌ | 中 |
| 时间旅行 | ✅ | ❌ | 中 |
| Context 管理 | ✅ | ❌ | 低 |
| 事件驱动 | ✅ | ❌ | 低 |
| 动态图 | ✅ | ❌ | 低 |
| 调试工具 | ✅ | ⚠️ 部分 | 低 |

## 关键差异说明

### 中断机制对比

| 特性 | nndeploy 当前 | LangGraph | 差距 |
|------|---------------|-----------|------|
| 终止执行 | ✅ 支持 | ✅ 支持 | - |
| 暂停等待 | ❌ 不支持 | ✅ 支持 | 🔴 关键缺失 |
| 继续执行 | ❌ 不支持 | ✅ 支持 | 🔴 关键缺失 |
| 保存状态 | ❌ 不支持 | ✅ 支持 | 🔴 关键缺失 |
| 恢复执行 | ❌ 不支持 | ✅ 支持 | 🔴 关键缺失 |
| 接收反馈 | ❌ 不支持 | ✅ 支持 | 🔴 关键缺失 |
| 超时控制 | ⚠️ 部分 | ✅ 支持 | 需完善 |
| 多种反馈类型 | ❌ 不支持 | ✅ 支持 | 需新增 |

## 关键文件清单

### 需要新建的文件（优先级排序）
```
framework/include/nndeploy/dag/
├── human_feedback.h       # 🔴 1. 人类反馈管理器
├── checkpoint.h           # 🔴 2. 检查点机制
├── callback.h             # 🔴 3. 回调系统
├── stream.h               # 高 4. 流式输出
├── message.h              # 中 5. 消息历史
├── context.h              # 低 6. 执行上下文
├── event.h                # 低 7. 事件定义
└── event_executor.h       # 低 8. 事件驱动执行器

framework/source/nndeploy/dag/
├── human_feedback.cc       # 🔴 1.
├── checkpoint.cc           # 🔴 2.
├── callback.cc             # 🔴 3.
├── stream.cc               # 高 4.
├── message.cc              # 中 5.
└── context.cc              # 低 6.
```

### 需要修改的文件（优先级排序）
```
framework/include/nndeploy/dag/
├── node.h                  # 🔴 1. 扩展状态机 + 反馈接口
├── executor.h              # 🔴 2. 集成反馈恢复逻辑
├── graph_runner.h          # 🔴 3. 集成反馈流程控制
├── edge.h                  # 中 添加 Reducer 支持
├── graph.h                 # 中 集成 Checkpoint、动态修改
└── checkpoint.h            # 高（新建）保存反馈状态
```

### 文件依赖关系图
```
                    ┌─────────────────┐
                    │  human_feedback │
                    │    (新建)       │
                    └────────┬────────┘
                             │
                ┌────────────┼────────────┐
                ▼            ▼            ▼
          ┌──────────┐  ┌──────────┐  ┌──────────┐
          │  node.h  │  │executor.h│  │checkpoint│
          │  (修改)  │  │ (修改)   │  │  (新建)  │
          └──────────┘  └──────────┘  └──────────┘
                │            │
                ▼            ▼
          ┌────────────────────────┐
          │   graph_runner.h       │
          │      (修改)            │
          └────────────────────────┘
```

## 实施建议

### 第一阶段：人类反馈能力 🔴（核心优先）
**目标**：实现完整的人类反馈机制，支持暂停-等待-继续流程

1. 扩展 Node 状态机（新增 Waiting/Paused 状态）
2. 实现 HumanFeedbackManager 反馈管理器
3. 实现 HumanReviewNode 专用审核节点
4. 集成 Executor 的反馈恢复逻辑
5. 基础 API 接口
6. 单元测试和示例

**预计工作量**：中等（1-2周）

---

### 第二阶段：Checkpoint 机制 🔴（依赖第一阶段）
**目标**：实现状态持久化，支持断点续执行

1. Checkpoint 基类设计
2. MemoryCheckpoint 实现
3. RedisCheckpoint 实现
4. 与人类反馈集成（保存等待状态）
5. GraphRunner 集成

**预计工作量**：中等（1-2周）

---

### 第三阶段：完整回调系统 🔴
**目标**：实现生命周期钩子

1. CallbackHandler 基类
2. 在 Executor 中集成回调触发
3. 支持多处理器注册

**预计工作量**：较小（3-5天）

---

### 第四阶段：流式输出机制
**目标**：支持实时进度推送

1. StreamHandler 接口
2. 事件流格式定义
3. Node 集成流式推送
4. WebSocket 集成（可选）

**预计工作量**：中等（1周）

---

### 第五阶段：Reducer + 消息历史
**目标**：灵活的状态更新和对话管理

1. Edge 添加 Reducer 支持
2. MessageHistory 类
3. 与 Checkpoint 集成

**预计工作量**：中等（1-2周）

---

### 第六阶段：时间旅行 + Context
**目标**：增强调试和状态管理

1. Checkpoint 扩展历史记录
2. TimeTravel 接口
3. Context 管理器

**预计工作量**：中等（1周）

---

### 第七阶段：事件驱动 + 动态图 + 调试工具
**目标**：扩展高级特性

1. Event 和 EventDrivenExecutor
2. 动态图修改 API
3. 可视化调试输出

**预计工作量**：较大（2-3周）

每阶段完成后建议编写完整的测试用例和文档示例。

## 人类反馈能力实现路线图

```
┌─────────────────────────────────────────────────────────────┐
│ 阶段 1: 核心框架                        │
│  ├─ Node 状态扩展                   │
│  ├─ HumanFeedbackManager 设计         │
│  └─ 基础数据结构定义                 │
├─────────────────────────────────────────────────────────────┤
│ 阶段 2: 请求与响应机制                     │
│  ├─ requestHumanFeedback() 实现      │
│  ├─ onHumanFeedback() 实现           │
│  └─ 同步/异步模式支持                 │
├─────────────────────────────────────────────────────────────┤
│ 阶段 3: Executor 集成                  │
│  ├─ resumeNode() 实现                │
│  ├─ 反馈类型处理逻辑                  │
│  └─ 状态恢复机制                      │
├─────────────────────────────────────────────────────────────┤
│ 阶段 4: GraphRunner 流程控制               │
│  ├─ 等待反馈逻辑                      │
│  ├─ 超时处理                          │
│  └─ 错误恢复                          │
├─────────────────────────────────────────────────────────────┤
│ 阶段 5: Checkpoint 集成                    │
│  ├─ 保存反馈请求                      │
│  ├─ 恢复时加载待处理反馈               │
│  └─ 断点续执行                        │
├─────────────────────────────────────────────────────────────┤
│ 阶段 6: API 与 Python 绑定                  │
│  ├─ C API 导出                       │
│  ├─ Python 绑定                      │
│  └─ Server 集成                      │
└─────────────────────────────────────────────────────────────┘
```

## 总结

### 核心问题
nndeploy DAG 虽然具备基础中断机制，但只能**终止执行**，无法实现**暂停-等待反馈-继续执行**的流程，这是与 LangGraph 人类反馈能力的关键差距。

### 解决方案
通过引入人类反馈能力，实现：
1. **状态扩展**：新增 Waiting/Paused 状态
2. **反馈管理器**：统一管理反馈请求和响应
3. **恢复机制**：支持 Approval/Correction/Skip/Reroute/Terminate 五种反馈类型
4. **Checkpoint 集成**：保存等待状态，支持断点续执行

### 应用场景
- 内容审核（图片/视频审核）
- AI 生成结果的人工确认
- 敏感操作的二次确认
- 参数调整后重新执行
- 多人协作工作流

### 预期收益
| 收益 | 说明 |
|------|------|
| 人工可控 | 关键节点可由人工决策 |
| 质量保障 | 通过审核提升输出质量 |
| 灵活调整 | 运行时动态修改参数 |
| 合规要求 | 满足监管审核要求 |
| 协作友好 | 支持多人协作流程 |
