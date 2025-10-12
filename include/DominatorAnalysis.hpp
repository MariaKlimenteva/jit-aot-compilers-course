#pragma once
#include <unordered_map>
#include <vector>
#include "BasicBlock.hpp"
#include "Graph.hpp"

struct NodeInfo{
    BasicBlock* parent{nullptr};
    size_t dfsnum{0};
    BasicBlock* idom{nullptr};
    BasicBlock* semidom{nullptr};
    BasicBlock* ancestor{nullptr};
    BasicBlock* label{nullptr};
    std::vector<BasicBlock*> bucket;
};
class DominatorAnalysis {
public:
    explicit DominatorAnalysis(Graph* graph) : graph_(graph) {}
    void Run();
    const std::vector<BasicBlock*>& getRPO() const;
    BasicBlock* GetIdom(BasicBlock* bb) const;
private:
    Graph* graph_;

    size_t dfsCounter_ = 0;
    
    std::unordered_map<BasicBlock*, NodeInfo> info_;
    std::vector<BasicBlock*> vertex_;
    
    void Dfs(BasicBlock* u, BasicBlock* p);
    void Link(BasicBlock* v, BasicBlock* w);
    BasicBlock* Eval(BasicBlock* v);
};