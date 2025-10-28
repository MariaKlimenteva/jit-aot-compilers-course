#pragma once
#include <unordered_map>
#include <vector>
#include <iomanip>
#include <sstream>
#include <iostream>
#include "BasicBlock.hpp"
#include "Graph.hpp"

struct NodeInfo{
    BasicBlock* parent{nullptr};
    size_t dfsnum{0};
    BasicBlock* idom{nullptr};
    BasicBlock* semidom{nullptr};
    BasicBlock* ancestor{nullptr};
    BasicBlock* label{nullptr};
    std::vector<BasicBlock*> bucket;

    std::string ToString() const {
        std::stringstream ss;
        ss << "dfsnum=" << dfsnum;
        ss << " parent=" << (parent ? std::to_string(parent->GetId()) : "null");
        ss << " idom=" << (idom ? std::to_string(idom->GetId()) : "null");
        ss << " semidom=" << (semidom ? std::to_string(semidom->GetId()) : "null");
        ss << " ancestor=" << (ancestor ? std::to_string(ancestor->GetId()) : "null");
        ss << " label=" << (label ? std::to_string(label->GetId()) : "null");
        ss << " bucket=[";
        for (size_t i = 0; i < bucket.size(); ++i) {
            if (i > 0) ss << ", ";
            ss << bucket[i]->GetId();
        }
        ss << "]";
        return ss.str();
    }

};
class DominatorAnalysis {
public:
    explicit DominatorAnalysis(Graph* graph) : graph_(graph) {}
    void Run();
    const std::vector<BasicBlock*>& getRPO() const;
    BasicBlock* GetIdom(BasicBlock* bb) const;

    void DumpNodeInfo(BasicBlock* bb = nullptr) const;
    void DumpDFSOrder() const;
    void DumpDominatorTree() const;
private:
    Graph* graph_;

    size_t dfsCounter_ = 0;
    
    std::unordered_map<BasicBlock*, NodeInfo> info_;
    std::vector<BasicBlock*> vertex_;
    
    void Dfs(BasicBlock* u, BasicBlock* p);
    void Link(BasicBlock* v, BasicBlock* w);
    BasicBlock* Eval(BasicBlock* v);
    void Compress(BasicBlock* v);

    std::string BBId(BasicBlock* bb) const;
    void DumpNodeInfoDetailed(BasicBlock* bb) const;
};