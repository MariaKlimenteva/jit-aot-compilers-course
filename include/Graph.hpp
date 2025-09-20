#pragma once

#include <memory>
#include <list>
#include "BasicBlock.hpp"

class Graph {
private:
    std::list<std::unique_ptr<BasicBlock>> blocks_;
public:
    Graph() = default;
    BasicBlock* CreateNewBasicBlock() {
        blocks_.emplace_back(std::make_unique<BasicBlock>(this));
        return blocks_.back().get();
    }

    const std::list<std::unique_ptr<BasicBlock>>& GetBlocks() const { return blocks_; }


};