# nndeploy 端侧 Agent 化改造方案（基于现有架构）

## 背景

nndeploy 当前架构已经具备：
- DAG 有向无环图执行
- Node/Edge 节点和边抽象
- Executor 执行器（串行/流水线并行/任务并行）
- Inference 多端推理（13+ 后端）
- Device 设备抽象
- Python 绑定层

目标是**基于现有架构**，不引入 LangGraph 等外部依赖，增强 nndeploy 使其更适合成为端侧 agent 框架。

## 现有架构能力分析

### 已具备的能力

| 能力 | 实现位置 | 说明 |
|------|-----------|------|
| DAG 执行 | framework/dag/graph.h | 串行/流水线并行/任务并行 |
| 节点抽象 | framework/dag/node.h | Node 基类，NodeDesc 描述 |
| 数据传输 | framework/dag/edge.h | Edge 边，支持队列 |
| 推理接口 | framework/inference/ | 统一接口，13+ 后端 |
| 设备抽象 | framework/device/ | CPU/GPU/NPU 统一接口 |
| 数据容器 | device/buffer/tensor/mat | Buffer/Tensor/Mat |
| Python API | python/nndeploy/ | 完整 Python 绑定 |
| JSON 序列化 | dag/node.py | 工作流保存/加载 |

### 缺少的 Agent 能力

| 能力 | 说明 |
|------|------|
| 集中式状态管理 | 节点间共享状态，而非仅通过 Edge 传递 |
| 节点工具 Schema | 导出为 LLM 可调用工具描述 |
| 条件分支执行 | 根据状态动态选择执行路径 |
| 对话消息历史 | 多轮对话记忆管理 |
| 流式输出支持 | 渐进式返回结果 |

## 增强方案（完全基于现有架构）

### 阶段一：状态管理模块（优先级：高）

**新增目录**：`python/nndeploy/agent/`

```
python/nndeploy/agent/
├── __init__.py
├── state.py          # 集中式状态管理
└── memory.py        # 消息历史管理
```

#### 1. AgentState 实现

```python
import json
from typing import Any, Optional, Dict, List
from nndeploy.base import Param

class AgentState:
    """
    集中式状态管理，与 DAG 的 Edge 传输并存

    设计原则：
    1. 端侧轻量，使用内存存储
    2. 可选文件持久化
    3. 与现有 DAG Edge 不冲突
    """

    def __init__(self, persist_path: Optional[str] = None):
        self._data: Dict[str, Any] = {}
        self._persist_path = persist_path

    def get(self, key: str, default: Any = None) -> Any:
        """获取状态值"""
        return self._data.get(key, default)

    def set(self, key: str, value: Any):
        """设置状态值"""
        self._data[key] = value

    def update(self, updates: Dict[str, Any]):
        """批量更新"""
        self._data.update(updates)

    def to_dict(self) -> Dict[str, Any]:
        """导出为字典"""
        return self._data.copy()

    def save(self, path: Optional[str] = None):
        """保存到文件"""
        filepath = path or self._persist_path
        if filepath:
            with open(filepath, 'w') as f:
                json.dump(self._data, f, indent=2)

    def load(self, path: str):
        """从文件加载"""
        with open(path, 'r') as f:
            self._data = json.load(f)

    def clear(self):
        """清空状态"""
        self._data.clear()
```

#### 2. ConversationMemory 实现

```python
from typing import List, Dict
from dataclasses import dataclass
from datetime import datetime

@dataclass
class Message:
    role: str        # "user" or "assistant"
    content: str     # 消息内容
    timestamp: float = None

class ConversationMemory:
    """
    对话记忆管理，支持多轮对话
    """

    def __init__(self, max_history: int = 10):
        self._messages: List[Message] = []
        self._max_history = max_history

    def add_message(self, role: str, content: str):
        """添加消息"""
        msg = Message(
            role=role,
            content=content,
            timestamp=datetime.now().timestamp()
        )
        self._messages.append(msg)

        # 限制历史长度
        if len(self._messages) > self._max_history:
            self._messages.pop(0)

    def get_history(self, limit: Optional[int] = None) -> List[Message]:
        """获取历史消息"""
        if limit is None:
            return self._messages.copy()
        return self._messages[-limit:]

    def to_context(self) -> str:
        """转换为 LLM 上下文字符串"""
        lines = []
        for msg in self._messages:
            lines.append(f"{msg.role}: {msg.content}")
        return "\n".join(lines)

    def clear(self):
        """清空历史"""
        self._messages.clear()
```

### 阶段二：节点工具化（优先级：高）

**新增文件**：`python/nndeploy/agent/tool.py`

```python
from typing import Dict, Any
from dataclasses import dataclass
import inspect

@dataclass
class ToolParameter:
    name: str
    type: str           # "string", "integer", "boolean", "object"
    description: str
    default: Any = None
    required: bool = False

@dataclass
class ToolSchema:
    name: str
    description: str
    parameters: Dict[str, Any]

class ToolExporter:
    """
    工具导出器，将 nndeploy Node 转换为工具 Schema
    """

    @staticmethod
    def from_function(func: callable) -> ToolSchema:
        """从函数推断工具 Schema"""
        sig = inspect.signature(func)

        params = {}
        for name, param in sig.parameters.items():
            annotation = param.annotation
            param_type = ToolExporter._infer_type(annotation)
            params[name] = ToolParameter(
                name=name,
                type=param_type,
                description=f"Parameter {name}",
                required=param.default == inspect.Parameter.empty
            ).__dict__

        return ToolSchema(
            name=func.__name__,
            description=func.__doc__ or "",
            parameters={
                "type": "object",
                "properties": params
            }
        )

    @staticmethod
    def from_node(node) -> ToolSchema:
        """从 nndeploy Node 推断工具 Schema"""
        # 获取节点描述
        desc = node.get_desc() or f"Node {node.get_name()}"

        # 获取输入边信息
        input_names = node.get_input_names()

        # 构建参数 Schema
        params = {}
        for name in input_names:
            params[name] = ToolParameter(
                name=name,
                type="string",  # 端侧简化处理
                description=f"Input {name}"
            ).__dict__

        return ToolSchema(
            name=node.get_name(),
            description=desc,
            parameters={
                "type": "object",
                "properties": params,
                "required": list(input_names)
            }
        )

    @staticmethod
    def _infer_type(annotation) -> str:
        """推断参数类型"""
        if annotation == str:
            return "string"
        elif annotation == int:
            return "integer"
        elif annotation == float:
            return "number"
        elif annotation == bool:
            return "boolean"
        else:
            return "object"
```

### 阶段三：条件分支节点（优先级：中）

**新增文件**：`python/nndeploy/agent/nodes/conditional_node.py`

```python
from nndeploy.dag import Graph, Edge
from .base import BaseAgentNode
from ..state import AgentState
from typing import Callable, Any

class ConditionalNode(BaseAgentNode):
    """
    条件分支节点，基于现有 DAG 的 Graph 嵌套实现

    设计：使用多个子图代表不同分支，根据条件选择执行
    """

    def __init__(self, name: str, condition_func: Callable[[AgentState], str]):
        super().__init__(name)
        self.condition_func = condition_func
        self.branches: Dict[str, Graph] = {}

    def add_branch(self, branch_name: str, graph: Graph):
        """添加分支子图"""
        self.branches[branch_name] = graph

    def _create_dag_node(self) -> Graph:
        """创建 DAG 节点（这里是主图容器）"""
        # 条件节点本身作为主图
        main_graph = Graph(self.name)

        # 将分支作为子节点添加到主图
        for branch_name, branch_graph in self.branches.items():
            # 将子图包装为节点
            branch_node = main_graph.create_node(
                f"branch_{branch_name}",
                f"Subgraph for {branch_name}"
            )

            # 设置子图的输入输出
            main_graph.add_node(branch_graph)

        return main_graph

    def forward(self, state: AgentState) -> Any:
        """执行条件分支"""
        # 调用条件函数
        branch_name = self.condition_func(state)

        # 查找并执行对应分支
        if branch_name in self.branches:
            branch_graph = self.branches[branch_name]
            # 执行分支子图
            branch_graph.run()
            return {"selected_branch": branch_name}
        else:
            return {"error": f"Unknown branch: {branch_name}"}
```

### 阶段四：增强的节点基类（优先级：高）

**新增文件**：`python/nndeploy/agent/nodes/base_agent_node.py`

```python
from nndeploy.dag import Node
from nndeploy.base import Param
from typing import Any, Optional
from ..state import AgentState
from ..tool import ToolExporter, ToolSchema

class BaseAgentNode:
    """
    增强的节点基类，基于现有 nndeploy.dag.Node

    新增能力：
    1. 状态管理集成
    2. 工具 Schema 导出
    3. Python 友好的 forward 接口
    """

    def __init__(self, name: str):
        self.name = name
        self._dag_node: Optional[Node] = None
        self._state: Optional[AgentState] = None

    def _create_dag_node(self) -> Node:
        """由子类实现，创建底层的 DAG 节点"""
        raise NotImplementedError("Subclasses must implement _create_dag_node")

    def _initialize(self, state: AgentState) -> Node:
        """初始化 DAG 节点"""
        if self._dag_node is None:
            self._dag_node = self._create_dag_node()
        self._state = state
        return self._dag_node

    def __call__(self, state: AgentState) -> AgentState:
        """执行入口（兼容 Agent 调用）"""
        node = self._initialize(state)

        # 从状态提取输入
        input_data = self._extract_input(state)

        # 执行业务逻辑
        output_data = self.forward(input_data)

        # 写入状态
        self._write_output(state, output_data)

        return state

    @abstractmethod
    def forward(self, input: Any) -> Any:
        """业务逻辑，由子类实现"""
        pass

    def _extract_input(self, state: AgentState) -> Any:
        """从状态提取输入"""
        # 默认从状态中获取以节点名为 key 的数据
        return state.get(f"input_{self.name}")

    def _write_output(self, state: AgentState, output: Any):
        """将输出写入状态"""
        state.set(f"output_{self.name}", output)

    def as_tool(self) -> ToolSchema:
        """导出为工具 Schema"""
        if self._dag_node is None:
            # 还未初始化，返回空的 Schema
            node_ref = type(self)(self.name, "temp")
            return ToolExporter.from_node(node_ref)
        return ToolExporter.from_node(self._dag_node)
```

### 阶段五：推理节点封装（优先级：高）

**新增文件**：`python/nndeploy/agent/nodes/inference_node.py`

```python
from nndeploy.infer import InferNode
from nndeploy.device import DeviceType
from .base_agent_node import BaseAgentNode
from typing import Any, Dict

class InferenceAgentNode(BaseAgentNode):
    """
    推理节点，直接封装 nndeploy 的 InferNode

    完全复用现有推理能力，仅增加 Agent 层接口
    """

    def __init__(self, name: str, model_path: str,
                 device: str = "cpu", **kwargs):
        super().__init__(name)
        self.model_path = model_path
        self.device = device
        self._infer_kwargs = kwargs

    def _create_dag_node(self) -> Node:
        """创建底层的 InferNode"""
        # 直接使用 nndeploy 的 InferNode
        from nndeploy.dag import Node, Edge

        # 创建输入边
        input_edge = Edge("input")

        # 创建输出边
        output_edge = Edge("output")

        # 创建推理节点
        infer_node = InferNode(self.name)
        infer_node.set_device_type(device=self.device)

        # 设置模型路径等参数
        param = nndeploy.base.Param()
        param.set_string_value("model_path", self.model_path)
        infer_node.set_param(param)

        # 设置输入输出
        infer_node.set_inputs([input_edge])
        infer_node.set_outputs([output_edge])

        return infer_node

    def forward(self, input: Any) -> Any:
        """执行推理（使用底层节点）"""
        if self._dag_node is None:
            raise RuntimeError("Node not initialized")

        # 底层 DAG 节点会在 run() 时执行推理
        # 这里只是返回输入，实际推理在 DAG 执行时进行
        return {"input": input}

    def as_tool(self):
        """工具 Schema"""
        return {
            "name": self.name,
            "description": f"运行 {self.name} 模型推理",
            "parameters": {
                "type": "object",
                "properties": {
                    "input": {
                        "type": "string",
                        "description": "推理输入数据"
                    }
                },
                "required": ["input"]
            }
        }
```

### 阶段六：Agent 编排器（优先级：高）

**新增文件**：`python/nndeploy/agent/agent.py`

```python
from nndeploy.dag import Graph, Edge
from .state import AgentState
from .memory import ConversationMemory
from .tool import ToolExporter
from typing import List, Dict, Any, Optional

class Agent:
    """
    Agent 编排器，基于现有 DAG Graph 实现

    设计原则：
    1. 直接使用 nndeploy.dag.Graph 作为执行引擎
    2. 提供状态管理能力
    3. 提供工具导出能力
    4. 保持端侧轻量特性
    """

    def __init__(self, name: str, max_history: int = 10):
        self.name = name
        self.state = AgentState()
        self.memory = ConversationMemory(max_history=max_history)
        self._graph: Optional[Graph] = None
        self._nodes: List[BaseAgentNode] = []

    def _initialize_graph(self) -> Graph:
        """初始化 DAG 图"""
        if self._graph is None:
            self._graph = Graph(self.name)
        return self._graph

    def add_node(self, node: BaseAgentNode) -> 'Agent':
        """添加节点"""
        graph = self._initialize_graph()

        # 创建底层 DAG 节点
        dag_node = node._create_dag_node()

        # 添加到图
        self._graph.add_node(dag_node)

        self._nodes.append(node)
        return self

    def add_edge(self, from_node_name: str, to_node_name: str):
        """添加边"""
        graph = self._initialize_graph()
        from_node = graph.get_node(from_node_name)
        to_node = graph.get_node(to_node_name)

        if from_node and to_node:
            # 创建边
            edge = Edge(f"{from_node_name}_{to_node_name}")
            graph.add_edge(edge)

    def set_input(self, input_data: Any):
        """设置初始输入"""
        self.state.set("input", input_data)

    def run(self, input_data: Optional[Any] = None) -> AgentState:
        """运行 Agent"""
        if input_data is not None:
            self.set_input(input_data)

        # 运行底层 DAG
        self._graph.init()
        self._graph.run()
        self._graph.deinit()

        return self.state

    def get_tools(self) -> List[Dict[str, Any]]:
        """获取所有节点作为工具的 Schema"""
        tools = []

        for node in self._nodes:
            try:
                schema = node.as_tool()
                tools.append(schema.__dict__)
            except Exception as e:
                print(f"Failed to export tool for {node.name}: {e}")

        return tools

    def get_state(self) -> AgentState:
        """获取当前状态"""
        return self.state

    def add_message(self, role: str, content: str):
        """添加对话消息"""
        self.memory.add_message(role, content)

    def get_history(self, limit: Optional[int] = None) -> List[Dict[str, Any]]:
        """获取对话历史"""
        messages = self.memory.get_history(limit)
        return [
            {"role": msg.role, "content": msg.content}
            for msg in messages
        ]

    def save_state(self, path: str):
        """保存状态到文件"""
        self.state.save(path)

    def load_state(self, path: str):
        """从文件加载状态"""
        self.state.load(path)

    def to_json(self) -> str:
        """导出为 JSON（使用现有序列化）"""
        if self._graph:
            return self._graph.serialize()
        return "{}"
```

### 阶段七：工作流 JSON 增强（优先级：中）

**新增文件**：`python/nndeploy/agent/workflow.py`

```python
import json
from typing import Dict, List
from .agent import Agent

class Workflow:
    """
    工作流管理，基于 nndeploy DAG JSON 格式

    增强：添加 Agent 状态、消息历史等元数据
    """

    @staticmethod
    def save(agent: Agent, path: str, include_state: bool = True):
        """保存 Agent 工作流"""
        workflow_data = {
            "dag": json.loads(agent.to_json()),
        }

        if include_state:
            workflow_data["state"] = agent.get_state().to_dict()

            history = agent.get_history()
            if history:
                workflow_data["messages"] = history

        with open(path, 'w') as f:
            json.dump(workflow_data, f, indent=2)

    @staticmethod
    def load(path: str) -> Dict[str, Any]:
        """加载工作流"""
        with open(path, 'r') as f:
            return json.load(f)
```

## 新增文件汇总

| 文件路径 | 行数估计 | 说明 |
|----------|-----------|------|
| `python/nndeploy/agent/__init__.py` | 50 | 模块入口 |
| `python/nndeploy/agent/state.py` | 100 | 状态管理 |
| `python/nndeploy/agent/memory.py` | 80 | 对话记忆 |
| `python/nndeploy/agent/tool.py` | 150 | 工具导出 |
| `python/nndeploy/agent/agent.py` | 200 | Agent 编排器 |
| `python/nndeploy/agent/workflow.py` | 80 | 工作流管理 |
| `python/nndeploy/agent/nodes/__init__.py` | 20 | 节点模块入口 |
| `python/nndeploy/agent/nodes/base_agent_node.py` | 120 | 节点基类 |
| `python/nndeploy/agent/nodes/inference_node.py` | 100 | 推理节点 |
| `python/nndeploy/agent/nodes/conditional_node.py` | 100 | 条件分支节点 |

**总计新增代码**：约 1000 行 Python 代码

## 使用示例

### 1. 基础 Agent 创建和运行

```python
from nndeploy.agent import Agent
from nndeploy.agent.nodes import InferenceAgentNode

# 创建 Agent
agent = Agent("vision_agent")

# 添加检测节点
detect = InferenceAgentNode(
    name="yolo_detect",
    model_path="models/yolo.onnx",
    device="cpu"
)
agent.add_node(detect)

# 添加分类节点
classify = InferenceAgentNode(
    name="resnet_classify",
    model_path="models/resnet.onnx",
    device="cpu"
)
agent.add_node(classify)

# 连接节点
agent.add_edge("input", "yolo_detect")
agent.add_edge("yolo_detect", "resnet_classify")

# 运行
result = agent.run({"image": "test.jpg"})
print(result.get("output_resnet_classify"))
```

### 2. 导出工具给 LLM

```python
tools = agent.get_tools()
# 输出:
# [
#   {
#     "name": "yolo_detect",
#     "description": "运行 yolo_detect 模型推理",
#     "parameters": {...}
#   },
#   {
#     "name": "resnet_classify",
#     "description": "运行 resnet_classify 模型推理",
#     "parameters": {...}
#   }
# ]
```

### 3. 条件分支

```python
from nndeploy.agent import Agent
from nndeploy.agent.nodes import ConditionalNode

agent = Agent("conditional_agent")

# 条件：根据状态选择分支
def condition(state):
    detected = state.get("output_detect")
    if detected and detected["count"] > 5:
        return "heavy"
    else:
        return "light"

# 添加条件节点
cond = ConditionalNode("route", condition_func=condition)
agent.add_node(cond)

# 添加分支
# ... 添加轻量处理分支
# ... 添加重量处理分支
```

### 4. 对话历史

```python
# 添加用户消息
agent.add_message("user", "请检测图像中的人脸")

# 添加助手消息
agent.add_message("assistant", "检测到 3 个人脸")

# 获取历史
history = agent.get_history()
# 输出最近 10 条消息
```

## 架构保证

### 完全复用的现有架构
1. **DAG 执行引擎** - 直接使用 `nndeploy.dag.Graph`
2. **推理能力** - 直接使用 `nndeploy.infer.InferNode`
3. **设备抽象** - 通过 InferNode 的 device_type 配置
4. **并行优化** - DAG 的串行/流水线并行/任务并行完全可用
5. **JSON 序列化** - 使用 Graph 的 serialize/save_file 方法
6. **Edge 数据传输** - DAG 的 Edge 机制

### 新增的 Agent 层（纯 Python，不修改 C++）
- Agent 编排器
- 状态管理器
- 对话记忆
- 工具导出器
- 条件分支节点
- 增强的节点基类

## 实施顺序

| 阶段 | 优先级 | 预计工时 |
|------|--------|----------|
| 阶段一：状态管理模块 | 高 | 2 天 |
| 阶段二：节点工具化 | 高 | 1 天 |
| 阶段三：条件分支节点 | 中 | 1 天 |
| 阶段四：增强的节点基类 | 高 | 1 天 |
| 阶段五：推理节点封装 | 高 | 1 天 |
| 阶段六：Agent 编排器 | 高 | 2 天 |
| 阶段七：工作流 JSON 增强 | 中 | 1 天 |
| **总计** | - | **9 天** |

## 验证方式

### 1. 单元测试
```python
# tests/test_agent_state.py
def test_state_basic():
    state = AgentState()
    state.set("key1", "value1")
    assert state.get("key1") == "value1"

def test_memory_basic():
    memory = ConversationMemory()
    memory.add_message("user", "hello")
    memory.add_message("assistant", "hi")
    assert len(memory.get_history()) == 2
```

### 2. 集成测试
```python
# tests/test_agent_integration.py
def test_agent_with_inference():
    agent = Agent("test")
    # 添加节点...
    agent.run({"input": "data"})
    # 验证输出
```

### 3. 工具导出测试
```python
def test_tool_export():
    tools = agent.get_tools()
    assert len(tools) > 0
    assert all("name" in tool for tool in tools)
    assert all("parameters" in tool for tool in tools)
```

### 4. 与现有 DAG 兼容测试
```python
# 确保 Agent 可以使用现有的 nndeploy 推理
from nndeploy.infer import InferNode

# 直接使用现有节点
graph = Graph("test")
infer = InferNode("test_infer")
graph.add_node(infer)
graph.init()
graph.run()
```
