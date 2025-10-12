#pragma once

#include <memory>
#include <list>
#include "BasicBlock.hpp"

class Graph {
private:
    std::list<std::unique_ptr<BasicBlock>> blocks_;
    int next_bb_id = 0;
    int next_inst_id_ = 0;
    BasicBlock* entry_block_ = nullptr;
public:
    Graph() = default;
    BasicBlock* CreateNewBasicBlock() {
        blocks_.emplace_back(std::make_unique<BasicBlock>(this, next_bb_id++));
        return blocks_.back().get();
    }
    int getNextInstructionId() {
        return next_inst_id_++;
    }
    void SetEntryBlock(BasicBlock* bb) {
        entry_block_ = bb;
    }
    BasicBlock* GetEntryBlock() const { return entry_block_; }
    const std::list<std::unique_ptr<BasicBlock>>& GetBlocks() const { return blocks_; }
    void Dump() const {
        for (auto& block : GetBlocks()) {
            block->Dump();
        }
    };
};