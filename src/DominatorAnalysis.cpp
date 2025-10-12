#include <algorithm>
#include "DominatorAnalysis.hpp"
#include "BasicBlock.hpp"

void DominatorAnalysis::Dfs(BasicBlock* u, BasicBlock* p) {
    info_[u].dfsnum = dfsCounter_;
    vertex_[dfsCounter_] = u;
    info_[u].label = u;
    info_[u].parent = p;
    info_[u].semidom = u;
    dfsCounter_++;

    for (auto* v : u->GetSuccs()) {
        if (info_.find(v) == info_.end()) {
            info_[v].parent = u;
            Dfs(v, u);
        }
    }
}

void DominatorAnalysis::Link(BasicBlock* v, BasicBlock* w) {
    info_.at(w).ancestor = v;
}

BasicBlock* DominatorAnalysis::Eval(BasicBlock* v) {
    auto& v_info = info_.at(v);
    if (v_info.ancestor == nullptr) {
        return v_info.label;
    }
    Eval(v_info.ancestor);
    auto* ancestor_label = info_.at(v_info.ancestor).label;
    auto* self_label = v_info.label;

    if (info_.at(info_.at(ancestor_label).semidom).dfsnum < info_.at(info_.at(self_label).semidom).dfsnum) {
        v_info.label = ancestor_label;
    }

    v_info.ancestor = info_.at(v_info.ancestor).ancestor;

    return v_info.label;
}

void DominatorAnalysis::Run() {
    BasicBlock* entryBlock = graph_->GetEntryBlock();
    if (entryBlock == nullptr) return;

    size_t numBlocks = graph_->GetBlocks().size();
    vertex_.resize(numBlocks);
    
    Dfs(entryBlock, nullptr);

    for (size_t i = dfsCounter_ - 1; i >= 1; --i) {
        BasicBlock* w = vertex_[i];
        if (w == nullptr) continue;
        auto& w_info = info_.at(w);

        for (BasicBlock* v : w->GetPreds()) {
            if (info_.find(v) == info_.end()) continue;

            BasicBlock* u = Eval(v);
            if (info_.at(info_.at(u).semidom).dfsnum < info_.at(w_info.semidom).dfsnum) {
                w_info.semidom = info_.at(u).semidom;
            }
        }
        info_.at(w_info.semidom).bucket.push_back(w);
        Link(w_info.parent, w);
        if (w_info.parent) {
            for (BasicBlock* v : info_.at(w_info.parent).bucket) {
                BasicBlock* u = Eval(v);
                if (info_.at(info_.at(u).semidom).semidom == info_.at(v).semidom) {
                    info_.at(v).idom = info_.at(v).semidom;
                } else {
                    info_.at(v).idom = u;
                }
            }
            info_.at(w_info.parent).bucket.clear();
        }
    }
    for (size_t i = 1; i < dfsCounter_; ++i) {
        BasicBlock* w = vertex_[i];
        if (!w) continue;
        
        auto& w_info = info_.at(w);
        if (w_info.idom != w_info.semidom) {
            w_info.idom = info_.at(w_info.idom).idom;
        }
    }

    info_[entryBlock].idom = nullptr;
}

BasicBlock* DominatorAnalysis::GetIdom(BasicBlock* bb) const {
    auto it = info_.find(bb);
    if (it != info_.end()) {
        return it->second.idom;
    }
    return nullptr;
}