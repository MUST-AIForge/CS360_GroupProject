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
echo "🔨 构建覆盖率计算器测试..."
cmake ..
if [[ "$OSTYPE" == "darwin"* ]]; then
    make clean
    make -j$(sysctl -n hw.ncpu) coverage_calculator_small_test
else
    make clean
    make -j$(nproc) coverage_calculator_small_test
fi

# 运行小规模测试
echo -e "\n🧪 运行小规模覆盖率计算测试..."
if [ -f "coverage_calculator_small_test" ]; then
    ./coverage_calculator_small_test
    SMALL_TEST_RESULT=$?
    if [ $SMALL_TEST_RESULT -eq 0 ]; then
        echo "✅ 小规模覆盖率计算测试通过"
    else
        echo "❌ 小规模覆盖率计算测试失败"
        exit 1
    fi
else
    echo "⚠️ 警告：找不到小规模测试可执行文件"
    exit 1
fi

 