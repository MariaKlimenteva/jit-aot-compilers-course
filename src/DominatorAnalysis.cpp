#include <algorithm>
#include "DominatorAnalysis.hpp"
#include "BasicBlock.hpp"

void DominatorAnalysis::DFS(BasicBlock* u) {
    dfnum_[u] = dfsCounter_;
    vertex_[dfsCounter_] = u;
    label_[u] = u;
    semi_[u] = u;
    ancestor_[u] = nullptr;
    dfsCounter_++;

    for (auto* v : u->GetSuccs()) {
        if (dfnum_.find(v) == dfnum_.end()) {
            parent_[v] = u;
            DFS(v);
        }
    }
}

void DominatorAnalysis::Run() {
    BasicBlock* entryBlock = graph_->GetEntryBlock();
    if (entryBlock == nullptr) return;

    size_t numBlocks = graph_->GetBlocks().size();
    vertex_.resize(numBlocks);
    
    dfsCounter_ = 0;
    DFS(entryBlock);
}