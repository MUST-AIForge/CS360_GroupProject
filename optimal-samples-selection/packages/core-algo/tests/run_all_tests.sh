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

# 构建整个项目
echo "🔨 构建所有测试..."
cmake ..
make clean
if [[ "$OSTYPE" == "darwin"* ]]; then
    make -j$(sysctl -n hw.ncpu)
else
    make -j$(nproc)
fi

# 运行所有测试
echo -e "\n🧪 运行所有测试..."

# 定义测试列表
declare -a tests=(
    "tests/coverage_calculator_test"
    "tests/mode_a_solver_test"
    "tests/mode_b_solver_test"
    "tests/mode_c_solver_test"
    "tests/combination_generator_test"
    "tests/set_operations_test"
)

# 测试结果统计
total_tests=0
passed_tests=0

# 运行每个测试
for test in "${tests[@]}"; do
    echo -e "\n📋 运行 $test..."
    if [ -f "$test" ]; then
        ./$test
        TEST_RESULT=$?
        if [ $TEST_RESULT -eq 0 ]; then
            echo "✅ $test 通过"
            ((passed_tests++))
        else
            echo "❌ $test 失败"
        fi
        ((total_tests++))
    else
        echo "⚠️ 警告: $test 可执行文件不存在"
    fi
done

# 打印测试结果摘要
echo -e "\n📊 测试结果摘要:"
echo "总测试数: $total_tests"
echo "通过测试: $passed_tests"
echo "失败测试: $((total_tests - passed_tests))"

# 设置退出状态
if [ $passed_tests -eq $total_tests ]; then
    echo -e "\n✨ 所有测试通过!"
    exit 0
else
    echo -e "\n❌ 部分测试失败"
    exit 1
fi 