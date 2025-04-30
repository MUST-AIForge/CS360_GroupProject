// 从 Wasm 模块导出的类型
export enum CoverageMode {
    CoverMinOneS = 'CoverMinOneS',   // Mode A: 至少覆盖一个s
    CoverMinNS = 'CoverMinNS',       // Mode B: 至少覆盖n个s
    CoverAllS = 'CoverAllS'          // Mode C: 覆盖所有s
}

export interface Solution {
    groups: number[][];      // 结果组集合
    totalGroups: number;     // 组数量
    computationTime: number; // 计算耗时（秒）
    isOptimal: boolean;      // 是否为最优解
}

// Wasm 模块接口
interface CoreAlgoModule {
    CoverageMode: typeof CoverageMode;
    findOptimalGroups(
        m: number,           // 样本总范围最大值
        samples: number[],   // 初始样本集合
        k: number,           // 输出组大小
        j: number,           // 约束参数
        s: number,           // 目标子集大小
        mode: CoverageMode,  // 覆盖模式
        N?: number          // Mode B需要的最少覆盖子集数
    ): Solution;
}

// 异步加载 Wasm 模块
let modulePromise: Promise<CoreAlgoModule>;

export async function initializeWasm(): Promise<void> {
    if (!modulePromise) {
        modulePromise = import('./core_algo_wasm.js').then(module => module.default);
    }
    await modulePromise;
}

export async function findOptimalGroups(
    m: number,
    samples: number[],
    k: number,
    j: number,
    s: number,
    mode: CoverageMode,
    N: number = 1
): Promise<Solution> {
    const module = await modulePromise;
    if (!module) {
        throw new Error('Wasm module not initialized. Call initializeWasm() first.');
    }
    
    return module.findOptimalGroups(m, samples, k, j, s, mode, N);
}

// 导出验证函数
export function validateInputs(
    m: number,
    samples: number[],
    k: number,
    j: number,
    s: number,
    mode: CoverageMode,
    N: number = 1
): boolean {
    // 基本范围检查
    if (m < 45 || m > 54) return false;
    if (samples.length < 7 || samples.length > 25) return false;
    if (k < 4 || k > 7) return false;
    if (s < 3 || s > 7) return false;
    if (s > j || j > k) return false;
    
    // 样本值检查
    if (samples.some(sample => sample < 1 || sample > m)) return false;
    
    // Mode B 的特殊检查
    if (mode === CoverageMode.CoverMinNS && N < 1) return false;
    
    return true;
} 