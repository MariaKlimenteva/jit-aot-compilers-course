#pragma once
#include "Graph.hpp"
#include "IRBuilder.hpp"
#include <map>

class Inliner {
public:
    explicit Inliner(Graph* caller) : caller_(caller) {}

    void Run() {
        bool changed = true;
        while (changed) {
            changed = false;
            CallInst* call_to_inline = nullptr;
            for (const auto& bb : caller_->GetBlocks()) {
                for (auto* inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
                    if (inst->GetOpcode() == Opcode::Call) {
                        call_to_inline = static_cast<CallInst*>(inst);
                        break;
                    }
                }
                if (call_to_inline) break;
            }

            if (call_to_inline) {
                InlineCall(call_to_inline);
                changed = true;
            }
        }
    }

private:
    Graph* caller_;

    void InlineCall(CallInst* call) {
        Graph* callee = call->GetCallee();
        BasicBlock* call_block = call->GetBasicBlock();
        BasicBlock* cont_block = call_block->SplitAfter(call);
        call_block->RemoveInst(call);

        std::map<BasicBlock*, BasicBlock*> bb_map;
        std::map<Instruction*, Instruction*> inst_map;

        int arg_idx = 0;
        for (auto* inst = callee->GetEntryBlock()->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::Param) {
                inst_map[inst] = call->GetInputs()[arg_idx++];
            }
        }

        IRBuilder builder(caller_);
        std::vector<ReturnInst*> cloned_returns;

        for (const auto& callee_bb_ptr : callee->GetBlocks()) {
            BasicBlock* callee_bb = callee_bb_ptr.get();
            BasicBlock* cloned_bb = caller_->CreateNewBasicBlock();
            bb_map[callee_bb] = cloned_bb;

            for (auto* inst = callee_bb->GetFirstPhi(); inst; inst = inst->GetNext()) {
                builder.SetInsertPoint(cloned_bb);
                inst_map[inst] = builder.CreatePhi(inst->GetType());
            }
            for (auto* inst = callee_bb->GetFirstInst(); inst; inst = inst->GetNext()) {
                if (inst->GetOpcode() == Opcode::Param) continue;
                
                builder.SetInsertPoint(cloned_bb);
                Instruction* cloned_inst = nullptr;

                if (inst->GetOpcode() == Opcode::Const) {
                    auto* c = static_cast<ConstantInst*>(inst);
                    builder.SetInsertPoint(caller_->GetEntryBlock());
                    cloned_inst = builder.CreateConstant(c->GetType(), c->GetValue());
                } else if (inst->GetOpcode() == Opcode::Add) {
                    cloned_inst = builder.CreateAdd(inst->GetInputs()[0], inst->GetInputs()[1]);
                } else if (inst->GetOpcode() == Opcode::Mul) {
                    cloned_inst = builder.CreateMul(inst->GetInputs()[0], inst->GetInputs()[1]);
                } else if (inst->GetOpcode() == Opcode::Cmp) {
                    cloned_inst = builder.CreateCmp(inst->GetInputs()[0], inst->GetInputs()[1]);
                } else if (inst->GetOpcode() == Opcode::Jump) {
                    auto* j = static_cast<JumpInst*>(inst);
                    cloned_inst = new JumpInst(caller_->getNextInstructionId(), cloned_bb, j->GetTarget());
                    cloned_bb->AppendInst(cloned_inst);
                } else if (inst->GetOpcode() == Opcode::If) {
                    auto* iff = static_cast<IfInst*>(inst);
                    cloned_inst = new IfInst(caller_->getNextInstructionId(), 
                    cloned_bb, iff->GetInputs()[0], iff->GetTrueTarget(),
                    iff->GetFalseTarget());
                    cloned_bb->AppendInst(cloned_inst);
                } else if (inst->GetOpcode() == Opcode::Ret) {
                    Instruction* ret_val = inst->GetInputs().empty() ? nullptr : inst->GetInputs()[0];
                    cloned_inst = builder.CreateReturn(ret_val);
                    cloned_returns.push_back(static_cast<ReturnInst*>(cloned_inst));
                }
                
                if (cloned_inst) {
                    inst_map[inst] = cloned_inst;
                }
            }
        }

        for (const auto& [old_inst, new_inst] : inst_map) {
            if (old_inst->GetOpcode() == Opcode::Param) continue; 
            if (new_inst->GetOpcode() == Opcode::Const) continue;
            for (size_t i = 0; i < new_inst->GetInputs().size(); ++i) {
                Instruction* old_in = new_inst->GetInputs()[i];
                if (inst_map.count(old_in)) {
                    new_inst->ReplaceInput(old_in, inst_map[old_in]);
                }
            }

            if (auto* jmp = dynamic_cast<JumpInst*>(new_inst)) {
                jmp->ReplaceTarget(bb_map[jmp->GetTarget()]);
            } else if (auto* iff = dynamic_cast<IfInst*>(new_inst)) {
                iff->ReplaceTargets(bb_map[iff->GetTrueTarget()], bb_map[iff->GetFalseTarget()]);
            } else if (auto* phi = dynamic_cast<PhiInst*>(new_inst)) {
                auto* old_phi = static_cast<PhiInst*>(old_inst);
                for (const auto& pair : old_phi->GetPhiInputs()) {
                    phi->AddPhiInput(bb_map[pair.first], inst_map[pair.second]);
                }
            }
        }

        for (const auto&[old_bb, new_bb] : bb_map) {
            for (auto* succ : old_bb->GetSuccs()) {
                new_bb->AddSucc(bb_map[succ]);
                bb_map[succ]->AddPred(new_bb);
            }
        }

        BasicBlock* callee_entry = bb_map[callee->GetEntryBlock()];
        builder.SetInsertPoint(call_block);
        builder.CreateJump(callee_entry);

        if (cloned_returns.size() == 1) {
            BasicBlock* ret_bb = cloned_returns[0]->GetBasicBlock();
            Instruction* ret_val = cloned_returns[0]->GetInputs().empty() ? nullptr : cloned_returns[0]->GetInputs()[0];
            
            ret_bb->RemoveInst(cloned_returns[0]);
            
            builder.SetInsertPoint(ret_bb);
            builder.CreateJump(cont_block);
            
            ReplaceAllUses(call, ret_val);
        } else if (cloned_returns.size() > 1) {
            builder.SetInsertPoint(cont_block);
            auto* phi = builder.CreatePhi(call->GetType());
            
            for (auto* ret : cloned_returns) {
                BasicBlock* ret_bb = ret->GetBasicBlock();
                Instruction* ret_val = ret->GetInputs()[0];
                
                phi->AddPhiInput(ret_bb, ret_val);
                ret_bb->RemoveInst(ret);
                
                builder.SetInsertPoint(ret_bb);
                builder.CreateJump(cont_block);
            }
            ReplaceAllUses(call, phi);
        }
    }

    void ReplaceAllUses(Instruction* oldInst, Instruction* newInst) {
        if (!oldInst || !newInst) return;
        for (auto& bb_ptr : caller_->GetBlocks()) {
            for (Instruction* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
                inst->ReplaceInput(oldInst, newInst);
            }
            for (Instruction* inst = bb_ptr->GetFirstPhi(); inst; inst = inst->GetNext()) {
                inst->ReplaceInput(oldInst, newInst);
            }
        }
    }
};