#pragma once

#include "DominatorAnalysis.hpp"
#include "Graph.hpp"
#include <stack>
#include <set>

struct BackEdge {
    BasicBlock* from;
    BasicBlock* to;  // header
    
    BackEdge(BasicBlock* f, BasicBlock* t) : from(f), to(t) {}
};

class Loop {
public:
    BasicBlock* header;
    std::set<BasicBlock*> blocks;
    std::vector<Loop*> sub_loops;
    Loop* parent_loop;
    
    Loop(BasicBlock* h) : header(h), parent_loop(nullptr) {}
    
    void AddBlock(BasicBlock* bb) { blocks.insert(bb); }
    void AddSubLoop(Loop* loop) { 
        sub_loops.push_back(loop); 
        loop->parent_loop = this;
    }
    
    bool Contains(BasicBlock* bb) const { 
        return blocks.find(bb) != blocks.end(); 
    }
};
class LoopAnalyzer {
private:
    Graph* graph_;
    DominatorAnalysis* dom_analysis_;
    
    enum class Color {
        WHITE, 
        GRAY,  
        BLACK   
    };
    
    std::unordered_map<BasicBlock*, Color> color_;
    std::vector<BackEdge> back_edges_;
    std::vector<Loop*> loops_;
    Loop* root_loop_;

    void FindLoops();
    void BuildLoopTree();
    void DFSVisit(BasicBlock* u, std::stack<BasicBlock*>& stack);
    void PrintLoopTree(Loop* loop, int depth);

public:
    explicit LoopAnalyzer(Graph* graph, DominatorAnalysis* dom_analysis) 
        : graph_(graph), dom_analysis_(dom_analysis), root_loop_(nullptr) {}
    
    void Analyze();
    const std::vector<Loop*>& GetLoops() const { return loops_; }
    Loop* GetRootLoop() const { return root_loop_; }
    
    void CollectBackEdges();
    void PrintBackEdges() const;
};