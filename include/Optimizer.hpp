#pragma once

#include "Graph.hpp"
#include "IRBuilder.hpp"

class Optimizer {
public:
    explicit Optimizer(Graph* graph) : graph_(graph) {}
    void Run();
private:
    Graph* graph_;
    bool TryConstantFolding(Instruction* inst);
    bool TryPeephole(Instruction* inst);
    void ReplaceAllUses(Instruction* oldInst, Instruction* newInst);
};

