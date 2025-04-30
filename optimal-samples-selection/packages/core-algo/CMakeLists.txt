# CMakeLists.txt for core-algo package

# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project definition
project(core-algo VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard (e.g., C++17)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags for optimization
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# Set RPATH settings
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Define include directory
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Find source files
file(GLOB_RECURSE SRCS
    "src/*.cpp"
    "src/algorithms/*.cpp"
    "src/models/*.cpp"
    "src/utils/*.cpp"
)

# 排除 test_reporter_main.cpp
list(FILTER SRCS EXCLUDE REGEX ".*test_reporter_main\\.cpp$")

# Define the core library (static library, suitable for Wasm compilation later)
add_library(core_algo_lib STATIC ${SRCS})

# Optional: Enable testing with CTest (requires test files in tests/)
enable_testing()

# Find GTest package
set(CMAKE_PREFIX_PATH "/usr/local/opt/googletest")
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 定义一个函数来创建测试套件
function(add_test_suite TEST_NAME TEST_SOURCE)
    # 创建常规测试目标
    add_executable(${TEST_NAME} 
        ${TEST_SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/algorithms/coverage_calculator_test_main.cpp
    )
    target_link_libraries(${TEST_NAME}
        PRIVATE
        core_algo_lib
        GTest::gtest
        GTest::gtest_main
    )
    target_include_directories(${TEST_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/src
    )
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})

    # 创建run_前缀的目标
    add_custom_target(run_${TEST_NAME}
        COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME}
        COMMAND ${CMAKE_CURRENT_BINARY_DIR}/${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Building and running ${TEST_NAME}"
    )
endfunction()

# 为每个测试文件创建单独的测试套件
add_test_suite(coverage_calculator_test tests/algorithms/coverage_calculator_test.cpp)
add_test_suite(combination_generator_test tests/algorithms/combination_generator_test.cpp)
add_test_suite(mode_a_solver_test tests/algorithms/mode_a_solver_test.cpp)
add_test_suite(mode_b_solver_test tests/algorithms/mode_b_solver_test.cpp)
add_test_suite(mode_c_solver_test tests/algorithms/mode_c_solver_test.cpp)
add_test_suite(set_operations_test tests/algorithms/set_operations_test.cpp)

# 添加simple_test可执行文件
add_executable(simple_test tests/simple_test.cpp)
target_link_libraries(simple_test 
    PRIVATE 
    core_algo_lib
    GTest::GTest 
)

# 添加包含目录
target_include_directories(simple_test 
    PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${GTEST_INCLUDE_DIRS}
)

# --- Add Emscripten specific configuration later for Wasm compilation ---

# Check if compiling with Emscripten
if(EMSCRIPTEN)
    message(STATUS "Emscripten detected. Configuring for WebAssembly.")

    # Define the Wasm output target
    # This compiles the static library into .wasm and .js glue code
    add_executable(core_algo_wasm ${SRCS})

    # Set Emscripten-specific compiler/linker flags
    set(CMAKE_EXECUTABLE_SUFFIX ".js") # Output will be .js file

    # Common flags for optimization and generating Wasm
    set(EMCC_FLAGS "-O3") # Optimization level
    set(EMCC_FLAGS "${EMCC_FLAGS} -s WASM=1") # Ensure WASM output
    set(EMCC_FLAGS "${EMCC_FLAGS} -s MODULARIZE=1") # Wrap output in a module
    set(EMCC_FLAGS "${EMCC_FLAGS} -s EXPORT_ES6=1") # Generate ES6 module
    set(EMCC_FLAGS "${EMCC_FLAGS} -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap','UTF8ToString']")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s ALLOW_MEMORY_GROWTH=1")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s EXPORTED_FUNCTIONS=['_malloc','_free']")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s NO_EXIT_RUNTIME=1")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s DISABLE_EXCEPTION_CATCHING=0")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s ASSERTIONS=1")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s SAFE_HEAP=1")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s ERROR_ON_UNDEFINED_SYMBOLS=1")
    set(EMCC_FLAGS "${EMCC_FLAGS} -s EMBIND_STD_STRING_IS_UTF8=1")
    set(EMCC_FLAGS "${EMCC_FLAGS} -lembind")

    # Set the flags for the Wasm target
    set_target_properties(core_algo_wasm PROPERTIES LINK_FLAGS ${EMCC_FLAGS})

else()
    message(STATUS "Not compiling with Emscripten. Configuring for native build.")
endif()
