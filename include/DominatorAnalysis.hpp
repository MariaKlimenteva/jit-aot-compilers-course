#pragma once
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "BasicBlock.hpp"
#include "Graph.hpp"
#include <set>
class DominatorAnalysis {
public:
    explicit DominatorAnalysis(Graph* graph) : graph_(graph) {}
    
    void Run();
    BasicBlock* GetIdom(BasicBlock* bb) const;
    bool Dominates(BasicBlock* dom, BasicBlock* node) const;
private:
    Graph* graph_;
    std::unordered_map<BasicBlock*, BasicBlock*> idoms_;
    std::unordered_map<BasicBlock*, std::set<BasicBlock*>> dominators_;
};