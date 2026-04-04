#!/bin/bash

# always 开发环境容器启动脚本
# 使用本地源码挂载方式，无需在容器内 git clone

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 默认参数
IMAGE_NAME="always:test"
CONTAINER_NAME="always_test"
HOST_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
WORKSPACE_DIR="/workspace"
GPU_ENABLED="--gpus all"
INTERACTIVE="-it"

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --no-gpu)
            GPU_ENABLED=""
            shift
            ;;
        --name)
            CONTAINER_NAME="$2"
            shift 2
            ;;
        --image)
            IMAGE_NAME="$2"
            shift 2
            ;;
        --non-interactive)
            INTERACTIVE=""
            shift
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo ""
            echo "选项:"
            echo "  --no-gpu          禁用 GPU"
            echo "  --name NAME       容器名称 (默认: always_test)"
            echo "  --image NAME      镜像名称 (默认: always:test)"
            echo "  --non-interactive 非交互模式"
            echo "  -h, --help        显示帮助信息"
            echo ""
            echo "示例:"
            echo "  $0                    # 启动交互式容器（带 GPU）"
            echo "  $0 --no-gpu          # 启动交互式容器（无 GPU）"
            echo "  $0 --name my_dev     # 使用自定义容器名称"
            exit 0
            ;;
        *)
            echo -e "${RED}未知参数: $1${NC}"
            echo "使用 -h 或 --help 查看帮助信息"
            exit 1
            ;;
    esac
done

# 检查镜像是否存在
if ! docker image inspect "$IMAGE_NAME" > /dev/null 2>&1; then
    echo -e "${YELLOW}镜像 $IMAGE_NAME 不存在，正在构建...${NC}"
    docker build -f docker/Dockerfile.ort_ov_trt_mnn.develop -t "$IMAGE_NAME" "$(dirname "$HOST_DIR")"
fi

# 检查是否已有同名容器在运行
if docker ps -q -f name="$CONTAINER_NAME" | grep -q .; then
    echo -e "${GREEN}容器 $CONTAINER_NAME 已在运行中${NC}"
    echo "使用以下命令进入容器:"
    echo "  docker exec -it $CONTAINER_NAME /bin/bash"
    exit 0
fi

# 检查是否有已停止的同名容器
if docker ps -aq -f name="$CONTAINER_NAME" | grep -q .; then
    echo -e "${YELLOW}发现已停止的容器 $CONTAINER_NAME，正在删除...${NC}"
    docker rm "$CONTAINER_NAME"
fi

# 启动容器
echo -e "${GREEN}正在启动容器...${NC}"
echo -e "  镜像: $IMAGE_NAME"
echo -e "  容器: $CONTAINER_NAME"
echo -e "  源码挂载: $HOST_DIR -> $WORKSPACE_DIR"

docker run $INTERACTIVE --rm \
    --name "$CONTAINER_NAME" \
    $GPU_ENABLED \
    --network host \
    -v "$HOST_DIR:$WORKSPACE_DIR" \
    -v "$HOST_DIR/build:$WORKSPACE_DIR/build" \
    -v "$HOST_DIR/tool/script/download:$WORKSPACE_DIR/tool/script/download" \
    -w "$WORKSPACE_DIR" \
    "$IMAGE_NAME" \
    /bin/bash
