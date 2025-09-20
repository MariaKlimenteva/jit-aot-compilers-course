#pragma once

#include "Graph.hpp"

class IRBuilder {
public:
    explicit IRBuilder(Graph* graph) : graph_(graph) {}

    void SetInsertPoint(BasicBlock* bb) {
        current_bb_ = bb;
    }

    ConstantInst* CreateConstant(Type type, ConstantInst::ValueType value) {
        auto* inst = new ConstantInst(type, current_bb_, value);
        current_bb_->AppendInst(inst);
        return inst;
    }

    BinaryInst* CreateAdd(Instruction* lhs, Instruction* rhs) {
        auto* inst = new BinaryInst(Opcode::Add, lhs->GetType(), current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }

    BinaryInst* CreateMul(Instruction* lhs, Instruction* rhs) {
        auto* inst = new BinaryInst(Opcode::Mul, lhs->GetType(), current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }
    
    BinaryInst* CreateCmp(Instruction* lhs, Instruction* rhs) {
        auto* inst = new BinaryInst(Opcode::Cmp, Type::int32, current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }
    
    JumpInst* CreateJump(BasicBlock* target) {
        auto* inst = new JumpInst(current_bb_, target);
        current_bb_->AppendInst(inst);
        current_bb_->AddSucc(target);
        target->AddPred(current_bb_);
        return inst;
    }

    IfInst* CreateIf(Instruction* cond, BasicBlock* true_target, BasicBlock* false_target) {
        auto* inst = new IfInst(current_bb_, cond, true_target, false_target);
        current_bb_->AppendInst(inst);
        current_bb_->AddSucc(true_target);
        current_bb_->AddSucc(false_target);
        true_target->AddPred(current_bb_);
        false_target->AddPred(current_bb_);
        return inst;
    }
    
    PhiInst* CreatePhi(Type type) {
        auto* inst = new PhiInst(type, current_bb_);
        current_bb_->AppendInst(inst);
        return inst;
    }
    
    ReturnInst* CreateReturn(Instruction* value = nullptr) {
        auto* inst = new ReturnInst(current_bb_, value);
        current_bb_->AppendInst(inst);
        return inst;
    }

    ParameterInst* CreateParameter(Type type) {
        auto* inst = new ParameterInst(type, current_bb_);
        current_bb_->AppendInst(inst);
        return inst;
    }

private:
    Graph* graph_;
    BasicBlock* current_bb_ = nullptr;
};