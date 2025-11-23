#include "LoopAnalyzer.hpp"
#include <algorithm>
#include <iostream>

void LoopAnalyzer::Run() {
    all_loops_.clear();
    color_.clear();
    loop_counter_ = 0;
    
    for (const auto& block_ptr : graph_->GetBlocks()) {
        color_[block_ptr.get()] = Color::WHITE;
    }

    BasicBlock* entry = graph_->GetEntryBlock();
    if (entry) {
        std::stack<BasicBlock*> stack;
        DFSVisit(entry, stack);
    }

    for (auto& loop : all_loops_) {
        PopulateLoop(loop.get());
    }

    BuildLoopTree();
}

void LoopAnalyzer::DFSVisit(BasicBlock* u, std::stack<BasicBlock*>& stack) {
    color_[u] = Color::GRAY;
    stack.push(u);

    for (BasicBlock* v : u->GetSuccs()) {
        if (color_[v] == Color::GRAY) {
            if (dom_analysis_->Dominates(v, u)) {
                Loop* loop = nullptr;
                for (auto& l : all_loops_) {
                    if (l->header == v) {
                        loop = l.get();
                        break;
                    }
                }
                
                if (!loop) {
                    all_loops_.push_back(std::make_unique<Loop>(v, ++loop_counter_));
                    loop = all_loops_.back().get();
                }
                
                loop->back_edges.push_back(u);
            }
        } else if (color_[v] == Color::WHITE) {
            DFSVisit(v, stack);
        }
    }

    color_[u] = Color::BLACK;
    stack.pop();
}

void LoopAnalyzer::PopulateLoop(Loop* loop) {
    std::vector<BasicBlock*> worklist;
    
    for (auto* latch : loop->back_edges) {
        if (latch != loop->header) {
            loop->blocks.insert(latch);
            worklist.push_back(latch);
        }
    }

    while (!worklist.empty()) {
        BasicBlock* curr = worklist.back();
        worklist.pop_back();

        for (auto* pred : curr->GetPreds()) {
            if (!loop->Contains(pred)) {
                loop->blocks.insert(pred);
                worklist.push_back(pred);
            }
        }
    }
}

void LoopAnalyzer::BuildLoopTree() {
    root_loop_ = std::make_unique<Loop>(nullptr, 0);

    std::vector<Loop*> sorted_loops;
    for (auto& l : all_loops_) {
        sorted_loops.push_back(l.get());
    }
    
    std::sort(sorted_loops.begin(), sorted_loops.end(), [](Loop* a, Loop* b) {
        return a->blocks.size() < b->blocks.size();
    });

    for (Loop* loop : sorted_loops) {
        Loop* parent = nullptr;
        
        for (Loop* candidate : sorted_loops) {
            if (candidate == loop) continue;
            
            if (candidate->Contains(loop->header)) {
                if (parent == nullptr || candidate->blocks.size() < parent->blocks.size()) {
                    parent = candidate;
                }
            }
        }

        if (parent) {
            loop->parent_loop = parent;
            parent->sub_loops.push_back(loop);
        } else {
            loop->parent_loop = root_loop_.get();
            root_loop_->sub_loops.push_back(loop);
        }
    }
}

void LoopAnalyzer::Dump() const {
    std::cout << "\n=== LOOP TREE DUMP ===" << std::endl;
    
    std::function<void(const Loop*, size_t)> print_recursive = 
        [&](const Loop* loop, size_t depth) {
            
        std::string indent(depth * 2, ' ');
        
        if (loop == root_loop_.get()) {
            std::cout << indent << "[Root Loop] (contains unmatched blocks and top-level loops)" << std::endl;
        } else {
            std::cout << indent << "L" << loop->id << ": Header BB" << loop->header->GetId();
            
            std::cout << ", BackEdges: {";
            for (auto* be : loop->back_edges) std::cout << "BB" << be->GetId() << " ";
            std::cout << "}";

            std::cout << ", Body: { ";
            for (auto* bb : loop->blocks) std::cout << bb->GetId() << " ";
            std::cout << "}" << std::endl;
        }

        for (const auto* sub : loop->sub_loops) {
            print_recursive(sub, depth + 1);
        }
    };

    if (root_loop_) {
        print_recursive(root_loop_.get(), 0);
    } else {
        std::cout << "No loops analysis performed or empty graph." << std::endl;
    }
    std::cout << "======================\n" << std::endl;
}
