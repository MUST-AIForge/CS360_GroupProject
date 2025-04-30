#!/bin/bash

# 设置颜色输出
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}开始安装 Emscripten SDK...${NC}"

# 检查 git 是否安装
if ! command -v git &> /dev/null; then
    echo "错误：git 未安装。请先安装 git。"
    exit 1
fi

# 检查 emsdk 目录是否存在
if [ ! -d "emsdk" ]; then
    echo "克隆 emsdk 仓库..."
    git clone https://github.com/emscripten-core/emsdk.git
fi

# 进入 emsdk 目录
cd emsdk || exit 1

# 拉取最新更新
git pull

# 安装最新版本
./emsdk install latest

# 激活最新版本
./emsdk activate latest

# 设置环境变量
source ./emsdk_env.sh

# 验证安装
if emcc --version; then
    echo -e "${GREEN}Emscripten SDK 安装成功！${NC}"
    echo -e "${BLUE}在新的终端会话中使用 Emscripten，请运行以下命令：${NC}"
    echo -e "cd $(pwd)"
    echo -e "source ./emsdk_env.sh"
else
    echo "安装验证失败，请检查错误信息。"
    exit 1
fi 