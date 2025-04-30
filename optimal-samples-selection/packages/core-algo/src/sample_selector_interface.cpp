#include "sample_selector_interface.hpp"
#include "mode_a_solver.hpp"
#include "mode_b_solver.hpp"
#include "mode_c_solver.hpp"
#include "combination_generator.hpp"
#include "set_operations.hpp"
#include "coverage_calculator.hpp"
#include <memory>
#include <stdexcept>

namespace core_algo {

DetailedSolution SampleSelectorInterface::run(
    char mode,
    int m,
    int n,
    int k,
    int s,
    int j,
    int N,
    const std::vector<int>& samples
) {
    Config config;
    auto combGen = std::shared_ptr<CombinationGenerator>(CombinationGenerator::create());
    auto setOps = std::shared_ptr<SetOperations>(SetOperations::create(config));
    auto covCalc = CoverageCalculator::create(config);
    DetailedSolution solution;
    
    switch (mode) {
        case 'a': {
            auto solver = createModeASetCoverSolver(combGen, setOps, std::move(covCalc), config);
            if (!solver) throw std::runtime_error("无法创建ModeA求解器");
            solution = solver->solve(m, n, samples, k, s, j);
            break;
        }
        case 'b': {
            auto solver = createModeBSetCoverSolver(combGen, setOps, config);
            if (!solver) throw std::runtime_error("无法创建ModeB求解器");
            solution = solver->solve(m, n, samples, k, s, j, N);
            break;
        }
        case 'c': {
            auto modeCSetCoverSolver = ModeCSetCoverSolver::createModeCSetCoverSolver(
                combGen,
                setOps,
                config
            );
            if (!modeCSetCoverSolver) throw std::runtime_error("无法创建ModeC求解器");
            solution = modeCSetCoverSolver->solve(m, n, samples, k, s, j);
            break;
        }
        default:
            throw std::invalid_argument("未知的mode参数，只能为a/b/c");
    }
    return solution;
}

} // namespace core_algo 