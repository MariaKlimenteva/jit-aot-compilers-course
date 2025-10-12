#pragma once
#include <unordered_map>
#include <vector>
#include "Graph.hpp"

class DominatorAnalysis {
public:
    explicit DominatorAnalysis(Graph* graph) : graph_(graph) {}
    void Run();
    const std::vector<BasicBlock*>& getRPO() const;
    BasicBlock* GetIdom(BasicBlock* bb) const;
private:
    Graph* graph_;
    std::vector<BasicBlock*> rpoOrder_;
    void DFS(BasicBlock* bb);
    size_t dfsCounter_ = 0;

    std::unordered_map<BasicBlock*, bool> visited_;
    std::unordered_map<BasicBlock*, BasicBlock*> parent_;
    std::unordered_map<BasicBlock*, BasicBlock*> idom_;
    std::unordered_map<BasicBlock*, BasicBlock*> semi_;
    std::unordered_map<BasicBlock*, BasicBlock*> ancestor_;
    std::unordered_map<BasicBlock*, BasicBlock*> label_;
    std::unordered_map<BasicBlock*, size_t> dfnum_;
    std::vector<BasicBlock*> vertex_;
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> bucket_;
};