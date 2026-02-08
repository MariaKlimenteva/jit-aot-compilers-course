#include "LivenessAnalysis.hpp"
#include <algorithm>

void LivenessAnalysis::ComputeRPO(BasicBlock* bb, std::set<BasicBlock*>& visited) {
    if (visited.count(bb)) return;
    visited.insert(bb);
    for (auto& successor : bb->GetSuccs()) {
        ComputeRPO(successor, visited);
    }
    linear_blocks_.push_back(bb);
}

void LivenessAnalysis::ComputeLinearOrder() {
    std::set<BasicBlock*> visited;
    linear_blocks_.clear();
    if (graph_->GetEntryBlock() == nullptr) return;
    ComputeRPO(graph_->GetEntryBlock(), visited);
    std::reverse(linear_blocks_.begin(), linear_blocks_.end());
}

void LivenessAnalysis::NumberInstructions() {
    int current_pos = 0;
    for (auto *bb: linear_blocks_) {
        for (auto *inst = bb->GetFirstPhi(); inst; inst = inst->GetNext()) {
            inst->SetLifePosition(current_pos);
            current_pos += 2;
        }
        for (auto *inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
            inst->SetLifePosition(current_pos);
            current_pos += 2;
        }
    }
}

void LivenessAnalysis::ComputeGlobalLiveness() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto it = linear_blocks_.rbegin(); it != linear_blocks_.rend(); ++it) {
            BasicBlock* bb = *it;
            std::set<int> new_live_out;

            for (BasicBlock* succ : bb->GetSuccs()) {
                const auto& succ_live_in = live_in_[succ];
                new_live_out.insert(succ_live_in.begin(), succ_live_in.end());
                for (auto* inst = succ->GetFirstPhi(); inst; inst = inst->GetNext()) {
                    auto* phi = static_cast<PhiInst*>(inst);
                    for (auto& [pred_block, val] : phi->GetPhiInputs()) {
                        if (pred_block == bb) {
                            new_live_out.insert(val->GetId());
                        }
                    }
                }
            }
            live_out_[bb] = new_live_out;
            std::set<int> current_live = new_live_out;
            for (auto* inst = bb->GetLastInst(); inst; inst = inst->GetPrev()) {
                current_live.erase(inst->GetId());
                for (auto* input : inst->GetInputs()) {
                    if (input->GetOpcode() != Opcode::Const && input->GetOpcode() != Opcode::Param) {
                        current_live.insert(input->GetId());
                    }
                }
            }
            if (live_in_[bb] != current_live) {
                live_in_[bb] = current_live;
                changed = true;
            }
        }
    }
}

std::pair<int, int> GetBlockRange(BasicBlock* bb) {
    int start = -1;
    int end = -1;

    if (auto* phi = bb->GetFirstPhi()) {
        start = phi->GetLifePosition();
    } else if (auto* inst = bb->GetFirstInst()) {
        start = inst->GetLifePosition();
    }

    if (auto* last = bb->GetLastInst()) {
        end = last->GetLifePosition() + 2;
    } else if (auto* last_phi = bb->GetLastPhi()) {
        end = last_phi->GetLifePosition() + 2;
    }

    if (start == -1) return {0, 0}; 
    return {start, end};
}

void LivenessAnalysis::BuildIntervals() {
    for (auto it = linear_blocks_.rbegin(); it != linear_blocks_.rend(); ++it) {
        BasicBlock* bb = *it;
        auto [block_from, block_to] = GetBlockRange(bb);

        if (live_out_.count(bb)) {
            for (int var_id : live_out_[bb]) {
                intervals_[var_id].reg_id = var_id;
                intervals_[var_id].AddRange(block_from, block_to);
            }
        }

        for (auto* inst = bb->GetLastInst(); inst; inst = inst->GetPrev()) {
            int pos = inst->GetLifePosition();

            if (inst->GetType() != Type::Unknown) {
                intervals_[inst->GetId()].reg_id = inst->GetId();
                intervals_[inst->GetId()].SetFrom(pos);
                intervals_[inst->GetId()].AddRange(pos, pos);
            }

            for (auto* input : inst->GetInputs()) {
                if (input->GetOpcode() != Opcode::Const && input->GetOpcode() != Opcode::Param) {
                    intervals_[input->GetId()].reg_id = input->GetId();
                    intervals_[input->GetId()].AddRange(block_from, pos);
                }
            }
        }

        for (auto* inst = bb->GetFirstPhi(); inst; inst = inst->GetNext()) {
            int pos = inst->GetLifePosition();
            intervals_[inst->GetId()].reg_id = inst->GetId();
            intervals_[inst->GetId()].SetFrom(pos);
        }
    }
}

void LivenessAnalysis::Run() {
    std::cout << "rkjga";
    ComputeLinearOrder();
    NumberInstructions();
    ComputeGlobalLiveness();
    BuildIntervals();
    Dump();
}