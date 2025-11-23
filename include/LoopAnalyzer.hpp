#pragma once

#include "DominatorAnalysis.hpp"
#include "Graph.hpp"
#include <stack>
#include <set>
#include <vector>
#include <unordered_map>
#include <memory>

struct Loop {
    BasicBlock* header = nullptr;
    std::vector<BasicBlock*> back_edges;
    std::set<BasicBlock*> blocks;
    
    Loop* parent_loop = nullptr;
    std::vector<Loop*> sub_loops;
    
    int id = 0;
    
    Loop(BasicBlock* h, int loop_id) : header(h), id(loop_id) {
        if (h) blocks.insert(h);
    }
    
    bool Contains(BasicBlock* bb) const {
        return blocks.find(bb) != blocks.end();
    }
};

class LoopAnalyzer {
public:
    explicit LoopAnalyzer(Graph* graph, DominatorAnalysis* dom_analysis) 
        : graph_(graph), dom_analysis_(dom_analysis) {}
    
    void Run();

    const std::vector<std::unique_ptr<Loop>>& GetLoops() const { return all_loops_; }
    Loop* GetRootLoop() const { return root_loop_.get(); }
    void Dump() const;
private:
    Graph* graph_;
    DominatorAnalysis* dom_analysis_;
    
    enum class Color { WHITE, GRAY, BLACK };
    std::unordered_map<BasicBlock*, Color> color_;
    
    std::vector<std::unique_ptr<Loop>> all_loops_;
    std::unique_ptr<Loop> root_loop_;
    int loop_counter_ = 0;

    void DFSVisit(BasicBlock* u, std::stack<BasicBlock*>& stack);
    void PopulateLoop(Loop* loop);
    void BuildLoopTree();
};