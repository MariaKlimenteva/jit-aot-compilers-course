#include "DominatorAnalysis.hpp"
#include <algorithm>

void DominatorAnalysis::Run() {
    idoms_.clear();
    dominators_.clear();
    
    const auto& blocks_list = graph_->GetBlocks();
    std::vector<BasicBlock*> blocks;
    for(auto& ptr : blocks_list) blocks.push_back(ptr.get());
    if (blocks.empty()) return;
    BasicBlock* entry = graph_->GetEntryBlock();

    std::set<BasicBlock*> all_blocks_set(blocks.begin(), blocks.end());
    
    for (auto* bb : blocks) {
        if (bb == entry) {
            dominators_[bb] = {entry};
        } else {
            dominators_[bb] = all_blocks_set;
        }
    }

    bool changed = true;
    while (changed) {
        changed = false;
        for (auto* bb : blocks) {
            if (bb == entry) continue;

            std::set<BasicBlock*> new_doms;
            bool first = true;

            if (bb->GetPreds().empty()) continue; 

            for (auto* pred : bb->GetPreds()) {
                if (dominators_.find(pred) == dominators_.end()) continue;

                if (first) {
                    new_doms = dominators_[pred];
                    first = false;
                } else {
                    std::set<BasicBlock*> intersection;
                    std::set_intersection(new_doms.begin(), new_doms.end(),
                                          dominators_[pred].begin(), dominators_[pred].end(),
                                          std::inserter(intersection, intersection.begin()));
                    new_doms = intersection;
                }
            }

            new_doms.insert(bb);

            if (new_doms != dominators_[bb]) {
                dominators_[bb] = new_doms;
                changed = true;
            }
        }
    }
    
    for (auto* bb : blocks) {
        if (bb == entry) {
            idoms_[bb] = nullptr;
            continue;
        }
        
        size_t target_size = dominators_[bb].size() - 1;
        BasicBlock* idom = nullptr;
        
        for (auto* dom : dominators_[bb]) {
            if (dom != bb && dominators_[dom].size() == target_size) {
                idom = dom;
                break;
            }
        }
        idoms_[bb] = idom;
    }
}

BasicBlock* DominatorAnalysis::GetIdom(BasicBlock* bb) const {
    auto it = idoms_.find(bb);
    return (it != idoms_.end()) ? it->second : nullptr;
}

bool DominatorAnalysis::Dominates(BasicBlock* dom, BasicBlock* node) const {
    if (!node || !dom) return false;

    auto it = dominators_.find(node);
    if (it == dominators_.end()) return false;
    return it->second.count(dom);
}