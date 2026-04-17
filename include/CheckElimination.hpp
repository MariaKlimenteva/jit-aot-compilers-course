#pragma once

#include "Graph.hpp"
#include "DominatorAnalysis.hpp"
#include <vector>
#include <algorithm>

class CheckElimination {
public:
    CheckElimination(Graph* graph, DominatorAnalysis* dom)
        : graph_(graph), dom_(dom) {}

    void Run();
    int GetRemovedCount() const { return removed_count_; }
private:
    Graph* graph_;
    DominatorAnalysis* dom_;
    int removed_count_ = 0;

    void ReplaceAllUses(Instruction* oldInst, Instruction* newInst);
    void RemoveDominatedChecks();
};
