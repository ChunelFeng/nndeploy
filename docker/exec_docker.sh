#!/bin/bash

# 用法:
# ./exec_docker.sh [--container 容器名] [--config 配置文件路径]
# 例如:
# ./exec_docker.sh --container my_container --config ../cmake/config_opencv.cmake

# 默认参数
CONTAINER_NAME="always_test"
CONFIG_FILE="../cmake/config.cmake"

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --container)
            CONTAINER_NAME="$2"
            shift 2
            ;;
        --config)
            CONFIG_FILE="$2"
            shift 2
            ;;
        -h|--help)
            echo "用法: $0 [--container 容器名] [--config 配置文件路径]"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

# 进入指定容器
docker exec -it "${CONTAINER_NAME}" /bin/bash -c "
    # 删除 build 目录
    rm -rf build

    # 新建 build 目录
    mkdir -p build

    # 复制 config.cmake 文件
    cp ${CONFIG_FILE} build/config.cmake

    # 进入 build 目录
    cd build

    # 配置 CMake
    cmake ..

    # 编译
    make -j\$(nproc)

    # 安装
    make install

    # 安装 Python 包
    cd ../python
    pip install -e .
"