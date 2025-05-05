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

# 只编译 preprocessor.cpp 和其依赖
set(SRCS
    "src/algorithms/preprocessor.cpp"
    "src/algorithms/combination_generator.cpp"
    "src/algorithms/set_operations.cpp"
    "src/algorithms/mode_a_solver.cpp"
    "src/algorithms/coverage_calculator.cpp"
)

# Define the core library
add_library(core_algo_lib STATIC ${SRCS})

# Enable testing with CTest
enable_testing()

# Find GTest package
set(CMAKE_PREFIX_PATH "/usr/local/opt/googletest")
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# 添加 mode_a_solver_test
add_executable(mode_a_solver_test tests/algorithms/mode_a_solver_test.cpp)
target_link_libraries(mode_a_solver_test
    PRIVATE
    core_algo_lib
    GTest::gtest
    GTest::gtest_main
)
target_include_directories(mode_a_solver_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_test(NAME mode_a_solver_test COMMAND mode_a_solver_test)

# 添加运行目标
add_custom_target(run_mode_a_solver_test
    COMMAND ${CMAKE_COMMAND} --build . --target mode_a_solver_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/mode_a_solver_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building and running mode_a_solver_test"
)

# 只添加 preprocessor_test
add_executable(preprocessor_test tests/algorithms/preprocessor_test.cpp)
target_link_libraries(preprocessor_test
    PRIVATE
    core_algo_lib
    GTest::gtest
)
target_include_directories(preprocessor_test
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
add_test(NAME preprocessor_test COMMAND preprocessor_test)

# 添加运行目标
add_custom_target(run_preprocessor_test
    COMMAND ${CMAKE_COMMAND} --build . --target preprocessor_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/preprocessor_test
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Building and running preprocessor_test"
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
