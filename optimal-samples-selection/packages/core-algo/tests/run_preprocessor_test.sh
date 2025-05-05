#!/bin/bash

# 设置错误时退出
set -e

# 彩色输出函数
print_info() {
    echo -e "\033[1;34m[INFO]\033[0m $1"
}

print_error() {
    echo -e "\033[1;31m[ERROR]\033[0m $1"
}

print_success() {
    echo -e "\033[1;32m[SUCCESS]\033[0m $1"
}

print_warning() {
    echo -e "\033[1;33m[WARNING]\033[0m $1"
}

# 显示帮助信息
show_help() {
    echo "用法: $0 [选项]"
    echo "选项:"
    echo "  -h, --help     显示此帮助信息"
    echo "  -m, --mode     指定要运行的测试模式 (all|a|b|c)"
    echo "                 all: 运行所有测试"
    echo "                 a: 只运行Mode A测试"
    echo "                 b: 只运行Mode B测试"
    echo "                 c: 只运行Mode C测试"
    echo "  -e, --edge     运行极限参数测试"
    echo "  -t, --test     指定要运行的具体测试用例"
    echo "                 例如: SmallParameterTest, MediumParameterTest"
    echo "  -f, --force    强制重新构建"
    echo "示例:"
    echo "  $0 -m b        # 只运行Mode B测试"
    echo "  $0 -e         # 运行所有极限参数测试"
    echo "  $0 -m b -e    # 运行Mode B的极限参数测试"
    echo "  $0 -t ModeBEdgeCaseTest  # 运行特定测试用例"
    echo "  $0 -f         # 强制重新构建并运行所有测试"
}

# 解析命令行参数
MODE="all"
FORCE_REBUILD=false
RUN_EDGE_CASES=false
SPECIFIC_TEST=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_help
            exit 0
            ;;
        -m|--mode)
            MODE="$2"
            shift 2
            ;;
        -e|--edge)
            RUN_EDGE_CASES=true
            shift
            ;;
        -t|--test)
            SPECIFIC_TEST="$2"
            shift 2
            ;;
        -f|--force)
            FORCE_REBUILD=true
            shift
            ;;
        *)
            print_error "未知选项: $1"
            show_help
            exit 1
            ;;
    esac
done

# 验证模式参数
if [[ ! "$MODE" =~ ^(all|a|b|c)$ ]]; then
    print_error "无效的模式: $MODE"
    show_help
    exit 1
fi

# 获取脚本所在目录的绝对路径
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# 获取项目根目录（core-algo）
PROJECT_ROOT="$SCRIPT_DIR/.."

# 检查CMakeLists.txt是否存在
if [ ! -f "$PROJECT_ROOT/CMakeLists.txt" ]; then
    print_error "在 $PROJECT_ROOT 目录下未找到 CMakeLists.txt"
    exit 1
fi

# 检查是否有写入权限
if [ ! -w "$PROJECT_ROOT" ]; then
    print_error "没有项目目录的写入权限: $PROJECT_ROOT"
    exit 1
fi

# 创建或清理构建目录
BUILD_DIR="$PROJECT_ROOT/build"
if [ "$FORCE_REBUILD" = true ] || [ ! -d "$BUILD_DIR" ]; then
    print_info "清理并重新创建构建目录..."
    rm -rf "$BUILD_DIR"
    mkdir -p "$BUILD_DIR" || {
        print_error "无法创建构建目录"
        exit 1
    }
fi

# 进入构建目录
cd "$BUILD_DIR"

# 检查源文件是否有更新
SRC_DIR="$PROJECT_ROOT/src/algorithms"
LAST_BUILD_TIME=0
if [ -f "CMakeCache.txt" ]; then
    LAST_BUILD_TIME=$(stat -f %m "CMakeCache.txt")
fi

NEED_REBUILD=false
if [ "$FORCE_REBUILD" = true ]; then
    NEED_REBUILD=true
else
    for file in "$SRC_DIR"/*.cpp; do
        if [ -f "$file" ]; then
            FILE_TIME=$(stat -f %m "$file")
            if [ $FILE_TIME -gt $LAST_BUILD_TIME ]; then
                print_info "检测到源文件更新: $(basename "$file")"
                NEED_REBUILD=true
                break
            fi
        fi
    done
fi

# 如果需要重新构建，则重新配置CMake
if [ "$NEED_REBUILD" = true ]; then
    print_info "配置CMake项目..."
    if ! cmake -DCMAKE_BUILD_TYPE=Debug "$PROJECT_ROOT"; then
        print_error "CMake配置失败"
        exit 1
    fi
    print_success "CMake配置完成"
fi

# 构建选定的目标
TARGETS="core_algo_lib preprocessor_test"
print_info "构建目标: $TARGETS..."
for target in $TARGETS; do
    print_info "构建 $target..."
    if ! make $target -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2); then
        print_error "构建 $target 失败"
        exit 1
    fi
done
print_success "构建完成"

# 运行测试
print_info "运行测试..."

# 构建测试过滤器
TEST_FILTER=""
if [ -n "$SPECIFIC_TEST" ]; then
    TEST_FILTER="PreprocessorTest.$SPECIFIC_TEST"
else
    case $MODE in
        "all")
            if [ "$RUN_EDGE_CASES" = true ]; then
                TEST_FILTER="PreprocessorTest.*EdgeCase*"
            fi
            ;;
        "a")
            if [ "$RUN_EDGE_CASES" = true ]; then
                TEST_FILTER="PreprocessorTest.ModeAEdgeCase*"
            else
                TEST_FILTER="PreprocessorTest.ModeAStrategyTest"
            fi
            ;;
        "b")
            if [ "$RUN_EDGE_CASES" = true ]; then
                TEST_FILTER="PreprocessorTest.ModeBEdgeCase*"
            else
                TEST_FILTER="PreprocessorTest.ModeBStrategyTest"
            fi
            ;;
        "c")
            if [ "$RUN_EDGE_CASES" = true ]; then
                TEST_FILTER="PreprocessorTest.ModeCEdgeCase*"
            else
                TEST_FILTER="PreprocessorTest.ModeCStrategyTest"
            fi
            ;;
    esac
fi

# 运行测试命令
if [ -n "$TEST_FILTER" ]; then
    print_info "运行测试过滤器: $TEST_FILTER"
    ./preprocessor_test --gtest_filter="$TEST_FILTER"
else
    ./preprocessor_test
fi

print_success "测试完成" 