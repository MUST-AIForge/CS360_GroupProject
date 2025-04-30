declare const CoreAlgoModule: {
    CoverageMode: {
        CoverMinOneS: 'CoverMinOneS';
        CoverMinNS: 'CoverMinNS';
        CoverAllS: 'CoverAllS';
    };
    findOptimalGroups(
        m: number,
        samples: number[],
        k: number,
        j: number,
        s: number,
        mode: 'CoverMinOneS' | 'CoverMinNS' | 'CoverAllS',
        N?: number
    ): {
        groups: number[][];
        totalGroups: number;
        computationTime: number;
        isOptimal: boolean;
    };
};

export default CoreAlgoModule; 