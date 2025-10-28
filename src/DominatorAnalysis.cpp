#include <algorithm>
#include "DominatorAnalysis.hpp"
#include "BasicBlock.hpp"

std::string DominatorAnalysis::BBId(BasicBlock* bb) const {
    if (!bb) return "null";
    return "BB" + std::to_string(bb->GetId());
}

void DominatorAnalysis::DumpNodeInfo(BasicBlock* bb) const {
    if (bb) {
        std::cout << "NodeInfo for " << BBId(bb) << ": " << info_.at(bb).ToString() << std::endl;
    } else {
        std::cout << "=== NodeInfo Dump (DFS order) ===" << std::endl;
        for (size_t i = 0; i < vertex_.size(); ++i) {
            BasicBlock* current_bb = vertex_[i];
            const NodeInfo& ni = info_.at(current_bb);
            std::cout << "  " << std::setw(3) << i << ": " << BBId(current_bb) 
                      << " -> " << ni.ToString() << std::endl;
        }
        std::cout << "=================================" << std::endl;
    }
}

void DominatorAnalysis::DumpNodeInfoDetailed(BasicBlock* bb) const {
    if (!bb) return;
    
    const NodeInfo& ni = info_.at(bb);
    std::cout << "=== Detailed NodeInfo for " << BBId(bb) << " ===" << std::endl;
    std::cout << "  DFS number: " << ni.dfsnum << std::endl;
    std::cout << "  Parent: " << BBId(ni.parent) << std::endl;
    std::cout << "  Immediate Dominator: " << BBId(ni.idom) << std::endl;
    std::cout << "  Semidominator: " << BBId(ni.semidom) << std::endl;
    std::cout << "  Ancestor: " << BBId(ni.ancestor) << std::endl;
    std::cout << "  Label: " << BBId(ni.label) << std::endl;
    
    std::cout << "  Bucket (" << ni.bucket.size() << " items): ";
    for (BasicBlock* b : ni.bucket) {
        std::cout << BBId(b) << " ";
    }
    std::cout << std::endl;
    
    std::cout << "  DFS ancestors: ";
    BasicBlock* current = bb;
    while (info_.at(current).parent) {
        current = info_.at(current).parent;
        std::cout << BBId(current) << " ";
    }
    std::cout << std::endl;
}

void DominatorAnalysis::DumpDFSOrder() const {
    std::cout << "=== DFS Order ===" << std::endl;
    for (size_t i = 0; i < vertex_.size(); ++i) {
        std::cout << "  " << i << ": " << BBId(vertex_[i]) 
                  << " (dfsnum=" << info_.at(vertex_[i]).dfsnum << ")" << std::endl;
    }
    std::cout << "=================" << std::endl;
}

void DominatorAnalysis::DumpDominatorTree() const {
    std::cout << "=== Dominator Tree ===" << std::endl;
    
    std::unordered_map<BasicBlock*, std::vector<BasicBlock*>> children;
    for (const auto& pair : info_) {
        BasicBlock* bb = pair.first;
        BasicBlock* idom = pair.second.idom;
        if (idom) {
            children[idom].push_back(bb);
        }
    }
    
    std::function<void(BasicBlock*, size_t)> print_tree = [&](BasicBlock* node, size_t depth) {
        std::string indent(depth * 2, ' ');
        std::cout << indent << BBId(node);
        if (depth == 0) std::cout << " (root)";
        std::cout << std::endl;
        
        for (BasicBlock* child : children[node]) {
            print_tree(child, depth + 1);
        }
    };
    
    BasicBlock* root = nullptr;
    for (const auto& pair : info_) {
        if (!pair.second.idom) {
            root = pair.first;
            break;
        }
    }
    
    if (root) {
        print_tree(root, 0);
    } else {
        std::cout << "  No root found!" << std::endl;
    }
    std::cout << "=====================" << std::endl;
}

void DominatorAnalysis::Dfs(BasicBlock* u, BasicBlock* p) {
    info_[u].dfsnum = dfsCounter_;
    vertex_[dfsCounter_] = u;
    info_[u].label = u;
    info_[u].parent = p;
    info_[u].semidom = u;
    dfsCounter_++;

    for (auto* v : u->GetSuccs()) {
        if (info_.find(v) == info_.end()) {
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
    
    Compress(v);
    return v_info.label;
}

void DominatorAnalysis::Compress(BasicBlock* v) {
    auto& v_info = info_.at(v);
    if (v_info.ancestor == nullptr) return;
    
    Compress(v_info.ancestor);
    
    auto& ancestor_info = info_.at(v_info.ancestor);
    if (info_[ancestor_info.label].semidom->GetId() < info_[v_info.label].semidom->GetId()) {
        v_info.label = ancestor_info.label;
    }
    v_info.ancestor = ancestor_info.ancestor;
}

void DominatorAnalysis::Run() {
    BasicBlock* entryBlock = graph_->GetEntryBlock();
    if (entryBlock == nullptr) return;

    size_t numBlocks = graph_->GetBlocks().size();
    vertex_.resize(numBlocks);
    
    Dfs(entryBlock, nullptr);
    
    std::cout << "=== After DFS ===" << std::endl;
    DumpDFSOrder();
    DumpNodeInfo();

    for (size_t i = dfsCounter_ - 1; i >= 1; --i) {
        BasicBlock* w = vertex_[i];
        if (w == nullptr) continue;
        auto& w_info = info_.at(w);

        for (BasicBlock* v : w->GetPreds()) {
            if (info_.find(v) == info_.end()) continue;

            BasicBlock* u = Eval(v);
            if (info_[info_[u].semidom].dfsnum < info_[w_info.semidom].dfsnum) {
                w_info.semidom = info_[u].semidom;
            }
        }
        
        info_[w_info.semidom].bucket.push_back(w);
        Link(w_info.parent, w);

        if (w_info.parent) {
            for (BasicBlock* v : info_[w_info.parent].bucket) {
                BasicBlock* u = Eval(v);
                if (info_[u].semidom == info_[v].semidom) {
                    info_[v].idom = info_[v].semidom;
                } else {
                    info_[v].idom = u;
                }
            }
            info_[w_info.parent].bucket.clear();
        }
    }

    std::cout << "=== After Steps 2-3 ===" << std::endl;
    DumpNodeInfo();

    for (size_t i = 1; i < dfsCounter_; ++i) {
        BasicBlock* w = vertex_[i];
        if (!w) continue;
        
        auto& w_info = info_.at(w);
        if (w_info.idom != w_info.semidom) {
            w_info.idom = info_[w_info.idom].idom;
        }
    }

    info_[entryBlock].idom = nullptr;
    
    std::cout << "=== Final Result ===" << std::endl;
    DumpNodeInfo();
    DumpDominatorTree();
}

BasicBlock* DominatorAnalysis::GetIdom(BasicBlock* bb) const {
    auto it = info_.find(bb);
    if (it != info_.end()) {
        return it->second.idom;
    }
    return nullptr;
}