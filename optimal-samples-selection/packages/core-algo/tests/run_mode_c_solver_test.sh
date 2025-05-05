#!/bin/bash

# 设置工作目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CORE_ALGO_DIR="$(dirname "$SCRIPT_DIR")"

# 清理之前的构建
echo "🧹 清理之前的构建..."
rm -rf "$CORE_ALGO_DIR/build"

# 创建构建目录
mkdir -p "$CORE_ALGO_DIR/build"
cd "$CORE_ALGO_DIR/build"

# 构建项目
echo "🔨 构建模式C求解器测试..."
cmake ..
make clean
make mode_c_solver_test

# 运行测试
echo -e "\n🧪 运行模式C求解器测试..."
if [ -f "tests/mode_c_solver_test" ]; then
    ./tests/mode_c_solver_test
    TEST_RESULT=$?
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ 模式C求解器测试通过"
    else
        echo "❌ 模式C求解器测试失败"
        exit 1
    fi
else
    echo "⚠️ 警告：找不到测试可执行文件"
    exit 1
fi 