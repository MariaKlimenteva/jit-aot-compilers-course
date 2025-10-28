#include "LoopAnalyzer.hpp"

void LoopAnalyzer::CollectBackEdges() {
    for (const auto& block_ptr : graph_->GetBlocks()) {
        BasicBlock* bb = block_ptr.get();
        color_[bb] = Color::WHITE;
    }
    
    std::stack<BasicBlock*> stack;
    BasicBlock* entry = graph_->GetEntryBlock();
    
    if (entry) {
        DFSVisit(entry, stack);
    }
}

void LoopAnalyzer::PrintBackEdges() const {
    std::cout << "=== Back Edges Found ===" << std::endl;
    if (back_edges_.empty()) {
        std::cout << "No back edges found" << std::endl;
    } else {
        for (const BackEdge& be : back_edges_) {
            std::cout << "BB" << be.from->GetId() << " -> BB" << be.to->GetId() << std::endl;
        }
    }
    std::cout << "========================" << std::endl;
}

void LoopAnalyzer::DFSVisit(BasicBlock* u, std::stack<BasicBlock*>& stack) {
    color_[u] = Color::GRAY;
    stack.push(u);
    
    std::cout << "Entering BB" << u->GetId() << " (mark GRAY)" << std::endl;
    std::cout << "  Stack: ";
    std::stack<BasicBlock*> temp = stack;
    while (!temp.empty()) {
        std::cout << "BB" << temp.top()->GetId() << " ";
        temp.pop();
    }
    std::cout << std::endl;
    
    for (BasicBlock* v : u->GetSuccs()) {
        std::cout << "  Checking edge BB" << u->GetId() << " -> BB" << v->GetId();
        
        if (color_[v] == Color::WHITE) {
            std::cout << " (WHITE -> tree edge)" << std::endl;
            DFSVisit(v, stack);
        } else if (color_[v] == Color::GRAY) {
            std::cout << " (GRAY -> potential back edge)" << std::endl;
            
            bool dominates = false;
            BasicBlock* current = u;
            while (current != nullptr) {
                if (current == v) {
                    dominates = true;
                    break;
                }
                current = dom_analysis_->GetIdom(current);
            }
            
            if (dominates) {
                std::cout << "  *** CONFIRMED BACK EDGE: BB" << u->GetId() 
                          << " -> BB" << v->GetId() << " ***" << std::endl;
                back_edges_.emplace_back(u, v);
            } else {
                std::cout << "  Rejected (not a dominator)" << std::endl;
            }
        } else {
            std::cout << " (BLACK -> forward/cross edge)" << std::endl;
        }
    }
    
    color_[u] = Color::BLACK;
    stack.pop();
    std::cout << "Leaving BB" << u->GetId() << " (mark BLACK)" << std::endl;
}
