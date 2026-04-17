#include "CheckElimination.hpp"
#include <map>

void CheckElimination::Run() {
    RemoveDominatedChecks();
}

static bool IsBefore(Instruction* first, Instruction* second) {
    if (first->GetBasicBlock() != second->GetBasicBlock()) return false;
    for (auto* inst = first->GetNext(); inst; inst = inst->GetNext()) {
        if (inst == second) return true;
    }
    return false;
}

void CheckElimination::ReplaceAllUses(Instruction* oldInst, Instruction* newInst) {
    auto users = oldInst->GetUsers();
    for (auto* user : users) {
        user->ReplaceInput(oldInst, newInst);
    }
}

void CheckElimination::RemoveDominatedChecks() {
    auto rpo = graph_->GetRPO();

    for (auto* bb : rpo) {
        Instruction* inst = bb->GetFirstInst();
        while (inst) {
            Instruction* next = inst->GetNext();
            Instruction* dominating = nullptr;

            if (inst->GetOpcode() == Opcode::NullCheck) {
                Instruction* ptr = inst->GetInputs()[0];
                for (auto* user : ptr->GetUsers()) {
                    if (user == inst) continue;
                    if (user->GetOpcode() == Opcode::NullCheck) {
                        BasicBlock* user_bb = user->GetBasicBlock();
                        if ((user_bb == bb && IsBefore(user, inst)) || 
                            (user_bb != bb && dom_->Dominates(user_bb, bb))) {
                            dominating = user;
                            break;
                        }
                    }
                }
            } else if (inst->GetOpcode() == Opcode::BoundsCheck) {
                Instruction* index = inst->GetInputs()[0];
                Instruction* length = inst->GetInputs()[1];
                for (auto* user : length->GetUsers()) {
                    if (user == inst) continue;
                    if (user->GetOpcode() == Opcode::BoundsCheck && user->GetInputs()[0] == index) {
                        BasicBlock* user_bb = user->GetBasicBlock();
                        if ((user_bb == bb && IsBefore(user, inst)) || 
                            (user_bb != bb && dom_->Dominates(user_bb, bb))) {
                            dominating = user;
                            break;
                        }
                    }
                }
            }

            if (dominating) {
                ReplaceAllUses(inst, dominating);
                bb->RemoveInst(inst);
                for (auto* input : inst->GetInputs()) {
                    if (input) input->RemoveUser(inst);
                }
                delete inst;
                removed_count_++;
            }
            inst = next;
        }
    }
}
