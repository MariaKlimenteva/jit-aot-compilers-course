#pragma once
#include "Graph.hpp"
#include "LoopAnalyzer.hpp"
#include <vector>
#include <set>

class LinearOrderBuilder {
public:
    explicit LinearOrderBuilder(Graph* graph, LoopAnalyzer* loops = nullptr) 
        : graph_(graph), loops_(loops) {}

    void Run() {
        linear_blocks_.clear();
        std::set<BasicBlock*> visited;
        if (graph_->GetEntryBlock()) {
            ComputeOrder(graph_->GetEntryBlock(), visited);
        }
    }

    const std::vector<BasicBlock*>& GetLinearOrder() const { 
        return linear_blocks_; 
    }

private:
    Graph* graph_;
    LoopAnalyzer* loops_;
    std::vector<BasicBlock*> linear_blocks_;

    void ComputeOrder(BasicBlock* bb, std::set<BasicBlock*>& visited) {
        if (visited.count(bb)) return;
        visited.insert(bb);

        linear_blocks_.push_back(bb);

        auto succs = bb->GetSuccs();
        std::sort(succs.begin(), succs.end(), [this](BasicBlock* a, BasicBlock* b) {
            return GetLoopDepth(a) > GetLoopDepth(b);
        });

        for (auto* succ : succs) {
            if (!IsBackEdge(bb, succ)) {
                ComputeOrder(succ, visited);
            }
        }
    }

    int GetLoopDepth(BasicBlock* bb) {
        if (!loops_) return 0;
        int depth = 0;
        for (const auto& loop : loops_->GetLoops()) {
            if (loop->Contains(bb)) depth++;
        }
        return depth;
    }

    bool IsBackEdge(BasicBlock* from, BasicBlock* to) {
        if (!loops_) return false;
        for (const auto& loop : loops_->GetLoops()) {
            if (loop->header == to && loop->Contains(from)) return true;
        }
        return false;
    }
};