#include "CheckElimination.hpp"
#include <iostream>

void CheckElimination::Run() {
    RemoveDominatedChecks();
    HoistChecksFromLoops();
}

bool CheckElimination::IsSameNullCheck(Instruction* a, Instruction* b) const {
    if (a->GetOpcode() != Opcode::NullCheck || b->GetOpcode() != Opcode::NullCheck) return false;
    return a->GetInputs()[0] == b->GetInputs()[0];
}

bool CheckElimination::IsSameBoundsCheck(Instruction* a, Instruction* b) const {
    if (a->GetOpcode() != Opcode::BoundsCheck || b->GetOpcode() != Opcode::BoundsCheck) return false;
    return a->GetInputs()[0] == b->GetInputs()[0] && a->GetInputs()[1] == b->GetInputs()[1];
}

static bool ComesBefore(Instruction* a, Instruction* b) {
    for (auto* cur = a; cur; cur = cur->GetNext()) {
        if (cur == b) return true;
    }
    return false;
}

bool CheckElimination::IsDominatedBySameCheck(Instruction* check) {
    for (auto& bb_ptr : graph_->GetBlocks()) {
        for (auto* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (inst == check) continue;
            if (IsSameNullCheck(inst, check) || IsSameBoundsCheck(inst, check)) {
                if (inst->GetBasicBlock() == check->GetBasicBlock()) {
                    if (!ComesBefore(inst, check)) continue;
                }
                if (dom_->Dominates(inst->GetBasicBlock(), check->GetBasicBlock())) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CheckElimination::ReplaceAllUses(Instruction* oldInst, Instruction* newInst) {
    for (auto& bb_ptr : graph_->GetBlocks()) {
        for (Instruction* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
            inst->ReplaceInput(oldInst, newInst);
        }
        for (Instruction* inst = bb_ptr->GetFirstPhi(); inst; inst = inst->GetNext()) {
            inst->ReplaceInput(oldInst, newInst);
        }
    }
}

void CheckElimination::RemoveDominatedChecks() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& bb_ptr : graph_->GetBlocks()) {
            std::vector<Instruction*> to_remove;
            for (auto* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
                if (inst->GetOpcode() != Opcode::NullCheck && inst->GetOpcode() != Opcode::BoundsCheck) continue;
                if (IsDominatedBySameCheck(inst)) {
                    to_remove.push_back(inst);
                }
            }
            for (auto* inst : to_remove) {
                Instruction* dominating = nullptr;
                for (auto& bb2 : graph_->GetBlocks()) {
                    for (auto* other = bb2->GetFirstInst(); other; other = other->GetNext()) {
                        if (other == inst) continue;
                        if (IsSameNullCheck(other, inst) || IsSameBoundsCheck(other, inst)) {
                            if (other->GetBasicBlock() == inst->GetBasicBlock()) {
                                if (!ComesBefore(other, inst)) continue;
                            }
                            if (dom_->Dominates(other->GetBasicBlock(), inst->GetBasicBlock())) {
                                dominating = other;
                                break;
                            }
                        }
                    }
                    if (dominating) break;
                }
                if (dominating) {
                    ReplaceAllUses(inst, dominating);
                    bb_ptr->RemoveInst(inst);
                    delete inst;
                    removed_count_++;
                    changed = true;
                }
            }
        }
    }
}

bool CheckElimination::IsLoopInvariant(Instruction* inst, Loop* loop) const {
    if (inst->GetOpcode() == Opcode::Const) return true;
    if (inst->GetOpcode() == Opcode::Param) return true;
    return !loop->Contains(inst->GetBasicBlock());
}

BasicBlock* CheckElimination::GetOrCreatePreheader(Loop* loop) {
    BasicBlock* header = loop->header;
    for (auto* pred : header->GetPreds()) {
        if (!loop->Contains(pred)) {
            if (pred->GetSuccs().size() == 1) {
                return pred;
            }
        }
    }

    BasicBlock* preheader = graph_->CreateNewBasicBlock();
    BasicBlock* outer_pred = nullptr;
    for (auto* pred : header->GetPreds()) {
        if (!loop->Contains(pred)) {
            outer_pred = pred;
            break;
        }
    }

    if (!outer_pred) return preheader;

    for (auto* inst = outer_pred->GetFirstInst(); inst; inst = inst->GetNext()) {
        if (inst->GetOpcode() == Opcode::Jump) {
            auto* jmp = static_cast<JumpInst*>(inst);
            if (jmp->GetTarget() == header) {
                jmp->ReplaceTarget(preheader);
                RedirectEdge(outer_pred, preheader, header);
                FixPhiBlocks(outer_pred, preheader, header);
                preheader->AddSucc(header);
                header->AddPred(preheader);
                break;
            }
        }
        if (inst->GetOpcode() == Opcode::If) {
            auto* if_inst = static_cast<IfInst*>(inst);
            if (if_inst->GetTrueTarget() == header || if_inst->GetFalseTarget() == header) {
                header->RemovePred(outer_pred);
                outer_pred->RemoveSucc(header);
                outer_pred->LinkTo(preheader);
                preheader->AddSucc(header);
                header->AddPred(preheader);
                FixPhiBlocks(outer_pred, preheader, header);
                break;
            }
        }
    }

    return preheader;
}

void CheckElimination::HoistChecksFromLoops() {
    if (!loops_) return;

    const auto& all_loops = loops_->GetLoops();
    for (auto& loop_ptr : all_loops) {
        Loop* loop = loop_ptr.get();

        std::vector<Instruction*> to_hoist;
        for (auto* bb : loop->blocks) {
            for (auto* inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
                if (inst->GetOpcode() != Opcode::NullCheck && inst->GetOpcode() != Opcode::BoundsCheck) continue;

                bool invariant = true;
                for (auto* input : inst->GetInputs()) {
                    if (!IsLoopInvariant(input, loop)) {
                        invariant = false;
                        break;
                    }
                }
                if (invariant) {
                    to_hoist.push_back(inst);
                }
            }
        }

        if (to_hoist.empty()) continue;

        BasicBlock* preheader = GetOrCreatePreheader(loop);
        for (auto* inst : to_hoist) {
            BasicBlock* orig_bb = inst->GetBasicBlock();
            orig_bb->RemoveInst(inst);

            Instruction* term = preheader->GetLastInst();
            if (term && (term->GetOpcode() == Opcode::Jump || term->GetOpcode() == Opcode::If)) {
                preheader->InsertInstBefore(inst, term);
            } else {
                preheader->AppendInst(inst);
            }
            hoisted_count_++;
        }
    }
}
