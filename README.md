# Optimal Samples Selection System

## 1. Project Goal

This project aims to develop a system that finds an optimal (or near-optimal) set of sample groups based on user-defined parameters, following the specifications outlined in the CS360/SE360 Group Project description. The system will provide a user interface for parameter input and result display, and will be packaged for offline use on both Windows and Android platforms.

## 2. Problem Definition (Current Understanding)

Given the following inputs:
*   `m`: The maximum value for the total sample range (integer, 45 <= m <= 54). Samples are represented by integers from 1 to `m`.
*   `n`: The number of initial samples selected from the total range (integer, 7 <= n <= 25).
*   `initialSamples`: A set/list of `n` unique integers representing the selected samples (from 1 to `m`).
*   `k`: The size of each output group (integer, 4 <= k <= 7, typically 6).
*   `j`: An integer constraint parameter (s <= j <= k).
*   `s`: The size of the target subsets involved in the coverage goal (integer, 3 <= s <= 7).
*   `coverageMode`: A user-selectable parameter indicating the desired coverage objective.

The system aims to find a **minimum-sized collection `C` of `k`-element subsets** (groups) derived from `initialSamples` that satisfies the user-selected `coverageMode`. The supported modes are:

*   **Mode A (CoverMinOneS):** Ensure that **for every `j`-element subset `J`** of `initialSamples`, there exists at least one `k`-element subset `G` in `C` such that `G` contains **at least ONE** `s`-element subset of `J`.
*   **Mode B (CoverMinNS):** Ensure that **for every `j`-element subset `J`** of `initialSamples`, there exists at least one `k`-element subset `G` in `C` such that `G` contains **at least N** `s`-element subsets of `J`. (Note: The value of N should be provided as a user input parameter).
*   **Mode C (CoverAllS):** Ensure that **every `s`-element subset** of `initialSamples` is contained within at least one of the `k`-element subsets in the collection `C`.

The parameter `j` acts as a constraint for `s` and is crucial for defining the scope in Modes A and B.

## 3. Solution Architecture

We will adopt a Monorepo structure to manage the different parts of the project. The core architecture is as follows:

*   **Core Algorithm:** Implemented in **C++** for performance, focusing on the set cover problem defined above.
*   **Cross-Platform Logic:** The C++ core algorithm will be compiled into **WebAssembly (Wasm)**, allowing it to be executed efficiently in JavaScript environments.
*   **Shared Wasm Module:** A dedicated package (`wasm-module`) will contain the compiled Wasm binary and a JavaScript/TypeScript wrapper to provide a clean API for the frontends.
*   **Mobile Application:** A **React Native** application (`mobile-app`) for Android, providing the UI and interacting with the Wasm module and local SQLite storage.
*   **Desktop Application:** An **Electron + React** application (`desktop-app`) for Windows, providing the UI and interacting with the Wasm module and local SQLite storage.
*   **Data Storage:** **SQLite** will be used on both platforms for storing generated results, accessed via appropriate libraries (e.g., `react-native-sqlite-storage`, Node.js `sqlite3`).

## 4. Core Algorithm Design

The specific algorithm depends on the selected `coverageMode`.

*   **For Mode C (CoverAllS):** This is a standard **Set Cover Problem**. We can use a **Greedy Algorithm**:
    1.  **Universe (U):** Generate all unique `s`-element subsets from `initialSamples` (\(\binom{n}{s}\) elements).
    2.  **Available Sets (S_k):** Generate all unique `k`-element subsets from `initialSamples` (\(\binom{n}{k}\) sets).
    3.  **Greedy Selection:** Repeatedly select the `k`-subset from S_k that covers the maximum number of *currently uncovered* `s`-subsets in U, until all elements in U are covered.

*   **For Mode A and Mode B:** These modes represent variations where we need to ensure coverage conditions for every `j`-subset of `initialSamples`. A greedy approach can be adapted to:
    *   **Mode A (CoverMinOneS):** Select `k`-subsets that satisfy the "at least one `s`-subset" condition for the maximum number of currently unsatisfied `j`-subsets.
    *   **Mode B (CoverMinNS):** Select `k`-subsets that satisfy the "at least N `s`-subsets" condition for the maximum number of currently unsatisfied `j`-subsets.
    *   Potential approaches might involve modifying the greedy scoring function to prioritize `k`-subsets based on how many `j`-subsets they help satisfy.
    *   **Further design and experimentation are required for an effective algorithm for these modes.**

## 5. Project Structure (Monorepo) / 项目结构

```
/optimal-samples-selection/                  # 最优样本选择系统根目录
├── package.json                            # Monorepo 根配置文件
├── PROGRESS.md                             # 项目进度跟踪文档
├── ISSUES.md                               # 问题跟踪文档
├── CURSOR_RULES.md                         # 开发规范文档
├── README.md                               # 项目说明文档
│
├── packages/                               # 子包目录
│   ├── core-algo/                         # C++ 核心算法实现
│   │   ├── src/                          # 源代码目录
│   │   │   ├── algorithms/               # 算法实现目录
│   │   │   │   ├── combination_generator.cpp    # 组合生成器实现
│   │   │   │   ├── set_operations.cpp          # 集合操作工具实现
│   │   │   │   ├── coverage_calculator.cpp     # 覆盖计算器实现
│   │   │   │   └── mode_c_solver.cpp          # Mode C 求解器实现
│   │   │   ├── models/                   # 数据模型目录
│   │   │   ├── utils/                    # 工具函数目录
│   │   │   ├── bindings.cpp             # WebAssembly 绑定实现
│   │   │   └── sample_selector.cpp      # 样本选择器主实现
│   │   │
│   │   ├── include/                      # 头文件目录
│   │   │   ├── combination_generator.hpp # 组合生成器接口
│   │   │   ├── set_operations.hpp       # 集合操作工具接口
│   │   │   ├── coverage_calculator.hpp  # 覆盖计算器接口
│   │   │   ├── sample_selector.hpp      # 样本选择器接口
│   │   │   └── types.hpp                # 核心数据类型定义
│   │   │
│   │   ├── tests/                        # 测试用例目录
│   │   ├── build/                        # 本地构建目录
│   │   ├── build_wasm/                   # WebAssembly 构建目录
│   │   └── CMakeLists.txt               # C++ 项目构建配置
│   │
│   └── wasm-module/                      # WebAssembly 模块
│       ├── src/                         # 源代码目录
│       │   ├── index.ts                # TypeScript 入口文件
│       │   └── core_algo_wasm.d.ts    # WebAssembly 类型定义
│       ├── dist/                        # 编译输出目录
│       └── binding/                     # Wasm 绑定代码
│
├── docs/                                  # 项目文档目录
│   ├── api/                             # API 文档
│   ├── algorithms/                      # 算法说明文档
│   └── user-guide/                      # 用户指南
│
├── scripts/                              # 项目脚本目录
│   ├── build.sh                        # 构建脚本
│   └── test.sh                         # 测试脚本
│
├── .gitignore                            # Git 忽略配置
└── LICENSE                               # 开源许可证
```

This document reflects our current understanding and plan. It may be updated as the project progresses. 

注意：
1. 每个子包都有自己的 `package.json` 和相关配置文件
2. `core-algo` 使用 CMake 构建系统
3. `wasm-module` 负责将 C++ 代码编译为 WebAssembly 并提供 JavaScript/TypeScript 接口
4. 移动应用和桌面应用都使用 SQLite 进行本地数据存储
5. 所有代码都遵循模块化设计原则，便于测试和维护 