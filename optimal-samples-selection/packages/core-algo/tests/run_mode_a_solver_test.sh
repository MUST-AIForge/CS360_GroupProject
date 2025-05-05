#!/bin/bash

# 设置工作目录
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
CORE_ALGO_DIR="$(dirname "$SCRIPT_DIR")"

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项] [测试名称]"
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  -l, --list     列出所有可用的测试用例"
    echo "  -a, --all      运行所有测试用例（默认）"
    echo ""
    echo "可用的测试用例:"
    echo "  MinimalValidParameters  - 最小有效参数测试"
    echo "  Example4Test           - 示例4测试"
    echo "  Example6Test           - 示例6测试"
    echo "  Example7Test           - 示例7测试"
    echo "  Example8Test           - 示例8测试"
    echo "  Example9Test           - 示例9测试"
}

# 列出所有测试用例
list_tests() {
    echo "可用的测试用例:"
    echo "- MinimalValidParameters  (最小有效参数测试)"
    echo "- Example4Test           (示例4测试)"
    echo "- Example6Test           (示例6测试)"
    echo "- Example7Test           (示例7测试)"
    echo "- Example8Test           (示例8测试)"
    echo "- Example9Test           (示例9测试)"
}

# 解析命令行参数
TEST_FILTER="*"
while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -l|--list)
            list_tests
            exit 0
            ;;
        -a|--all)
            TEST_FILTER="*"
            shift
            ;;
        *)
            if [[ -n "$1" ]]; then
                TEST_FILTER="ModeASetCoverSolverTest.$1"
            fi
            shift
            ;;
    esac
done

# 清理之前的构建
echo "🧹 清理之前的构建..."
rm -rf "$CORE_ALGO_DIR/build"

# 创建构建目录
mkdir -p "$CORE_ALGO_DIR/build"
cd "$CORE_ALGO_DIR/build"

# 构建项目
echo "🔨 构建模式A求解器测试..."
cmake ..
make clean
make mode_a_solver_test

# 运行测试
echo -e "\n🧪 运行模式A求解器测试..."
if [ -f "mode_a_solver_test" ]; then
    if [ "$TEST_FILTER" = "*" ]; then
        echo "运行所有测试用例..."
        ./mode_a_solver_test
    else
        echo "运行测试用例: $TEST_FILTER"
        ./mode_a_solver_test --gtest_filter="$TEST_FILTER"
    fi
    
    TEST_RESULT=$?
    if [ $TEST_RESULT -eq 0 ]; then
        echo "✅ 模式A求解器测试通过"
    else
        echo "❌ 模式A求解器测试失败"
        exit 1
    fi
else
    echo "⚠️ 警告：找不到测试可执行文件"
    exit 1
fi 