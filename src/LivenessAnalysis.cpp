#include "LivenessAnalysis.hpp"
#include <algorithm>

static bool IsTrackable(Instruction* inst) {
    return inst && inst->GetOpcode() != Opcode::Const && inst->GetType() != Type::Unknown;
}



void LivenessAnalysis::NumberInstructions() {
    int current_pos = 0;
    for (auto *bb: linear_blocks_) {
        auto number = [&](Instruction* inst) {
            if (inst) { inst->SetLifePosition(current_pos); current_pos += 2; }
        };
        for (auto *i = bb->GetFirstPhi(); i; i = i->GetNext()) number(i);
        for (auto *i = bb->GetFirstInst(); i; i = i->GetNext()) number(i);
    }
}

void LivenessAnalysis::BuildIntervals() {
    intervals_.clear();
    live_in_.clear();

    std::map<BasicBlock*, BasicBlock*> loop_headers;
    std::map<BasicBlock*, int> bb_pos;
    for(size_t i = 0; i < linear_blocks_.size(); ++i) bb_pos[linear_blocks_[i]] = i;

    for (auto* bb : linear_blocks_) {
        for (auto* succ : bb->GetSuccs()) {
            if (bb_pos.count(succ) && bb_pos[succ] <= bb_pos[bb]) {
                if (loop_headers.find(succ) == loop_headers.end() || 
                    bb_pos[bb] > bb_pos[loop_headers[succ]]) {
                    loop_headers[succ] = bb;
                }
            }
        }
    }

    for (auto it = linear_blocks_.rbegin(); it != linear_blocks_.rend(); ++it) {
        BasicBlock* b = *it;

        int b_from = -1;
        Instruction* first = b->GetFirstPhi() ? b->GetFirstPhi() : b->GetFirstInst();
        if (first) b_from = first->GetLifePosition();
        
        int b_to = b_from;
        Instruction* last = b->GetLastInst() ? b->GetLastInst() : b->GetLastPhi();
        if (last) b_to = last->GetLifePosition() + 2;

        if (b_from == -1) continue;

        std::set<int> live;
        for (auto* succ : b->GetSuccs()) {
             if (live_in_.count(succ)) {
                 live.insert(live_in_[succ].begin(), live_in_[succ].end());
             }

             for (auto* inst = succ->GetFirstPhi(); inst; inst = inst->GetNext()) {
                 auto* phi = static_cast<PhiInst*>(inst);
                 for (const auto& pair : phi->GetPhiInputs()) {
                     if (pair.first == b && IsTrackable(pair.second)) {
                         live.insert(pair.second->GetId());
                     }
                 }
             }
        }

        for (int opd : live) {
            intervals_[opd].reg_id = opd;
            intervals_[opd].AddRange(b_from, b_to);
        }

        auto process = [&](Instruction* inst) {
            if (!inst || !IsTrackable(inst)) return;
            
            if (inst->GetOpcode() != Opcode::Phi) {
                intervals_[inst->GetId()].reg_id = inst->GetId();
                intervals_[inst->GetId()].SetFrom(inst->GetLifePosition());
                live.erase(inst->GetId());
            }

            for (auto* input : inst->GetInputs()) {
                if (IsTrackable(input)) {
                    intervals_[input->GetId()].reg_id = input->GetId();
                    intervals_[input->GetId()].AddRange(b_from, inst->GetLifePosition());
                    live.insert(input->GetId());
                }
            }
        };

        for (auto* i = b->GetLastInst(); i; i = i->GetPrev()) process(i);

        for (auto* i = b->GetFirstPhi(); i; i = i->GetNext()) {
            if (IsTrackable(i)) {
                live.erase(i->GetId());
                intervals_[i->GetId()].reg_id = i->GetId();
                intervals_[i->GetId()].SetFrom(b_from);
            }
        }

        if (loop_headers.count(b)) {
            BasicBlock* loop_end = loop_headers[b];
            Instruction* end_inst = loop_end->GetLastInst() ? loop_end->GetLastInst() : loop_end->GetLastPhi();
            int loop_end_pos = end_inst ? end_inst->GetLifePosition() + 2 : b_to;
            
            for (int opd : live) {
                intervals_[opd].AddRange(b_from, loop_end_pos);
            }
        }

        live_in_[b] = live;
    }
}

void LivenessAnalysis::Run() {
    if (linear_blocks_.empty() && graph_->GetEntryBlock() != nullptr) {
        std::cerr << "Warning: Linear order is empty!" << std::endl;
    }
    
    NumberInstructions();
    BuildIntervals();
    Dump();
}