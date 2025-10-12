#pragma once

#include "Graph.hpp"

class DominatorAnalysis {
public:
    DominatorAnalysis(Graph* graph) : graph_(graph) {}
    void Run() {}
private:
    Graph* graph_;
};