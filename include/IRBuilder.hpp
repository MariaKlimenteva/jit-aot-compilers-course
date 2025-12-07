#pragma once

#include "Graph.hpp"

class IRBuilder {
private:
    Graph* graph_;
    BasicBlock* current_bb_ = nullptr;

    void CheckInsertPoint() const {
        if (current_bb_ == nullptr) {
            throw std::runtime_error("IRBuilder: no basic block set for insertion");
        }
    }

public:
    explicit IRBuilder(Graph* graph) : graph_(graph) {}

    void SetInsertPoint(BasicBlock* bb) {
        current_bb_ = bb;
    }

    ConstantInst* CreateConstant(Type type, ConstantInst::ValueType value) {
        CheckInsertPoint();
        auto* inst = new ConstantInst(graph_->getNextInstructionId(), type, current_bb_, value);
        current_bb_->AppendInst(inst);
        return inst;
    }

    BinaryInst* CreateAdd(Instruction* lhs, Instruction* rhs) {
        CheckInsertPoint();
        auto* inst = new BinaryInst(graph_->getNextInstructionId(), Opcode::Add, lhs->GetType(), 
            current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }

    BinaryInst* CreateMul(Instruction* lhs, Instruction* rhs) {
        CheckInsertPoint();
        auto* inst = new BinaryInst(graph_->getNextInstructionId(), Opcode::Mul, lhs->GetType(), 
            current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }
    
    BinaryInst* CreateCmp(Instruction* lhs, Instruction* rhs) {
        CheckInsertPoint();
        auto* inst = new BinaryInst(graph_->getNextInstructionId(), Opcode::Cmp, Type::int32, 
            current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }
    
    JumpInst* CreateJump(BasicBlock* target) {
        CheckInsertPoint();
        auto* inst = new JumpInst(graph_->getNextInstructionId(), current_bb_, target);
        current_bb_->AppendInst(inst);
        current_bb_->AddSucc(target);
        target->AddPred(current_bb_);
        return inst;
    }

    IfInst* CreateIf(Instruction* cond, BasicBlock* true_target, BasicBlock* false_target) {
        CheckInsertPoint();
        auto* inst = new IfInst(graph_->getNextInstructionId(), current_bb_, cond, true_target, false_target);
        current_bb_->AppendInst(inst);
        current_bb_->AddSucc(true_target);
        current_bb_->AddSucc(false_target);
        true_target->AddPred(current_bb_);
        false_target->AddPred(current_bb_);
        return inst;
    }
    
    PhiInst* CreatePhi(Type type) {
        CheckInsertPoint();
        auto* inst = new PhiInst(graph_->getNextInstructionId(), type, current_bb_);
        current_bb_->AppendInst(inst);
        return inst;
    }
    
    ReturnInst* CreateReturn(Instruction* value = nullptr) {
        CheckInsertPoint();
        auto* inst = new ReturnInst(graph_->getNextInstructionId(), current_bb_, value);
        current_bb_->AppendInst(inst);
        return inst;
    }

    ParameterInst* CreateParameter(Type type) {
        CheckInsertPoint();
        auto* inst = new ParameterInst(graph_->getNextInstructionId(), type, current_bb_);
        current_bb_->AppendInst(inst);
        return inst;
    }

    BinaryInst* CreateOr(Instruction* lhs, Instruction* rhs) {
        CheckInsertPoint();
        auto* inst = new BinaryInst(graph_->getNextInstructionId(), Opcode::Or, lhs->GetType(),
            current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }

    BinaryInst* CreateShr(Instruction* lhs, Instruction* rhs) {
        CheckInsertPoint();
        auto* inst = new BinaryInst(graph_->getNextInstructionId(), Opcode::AShr, 
            lhs->GetType(), current_bb_, lhs, rhs);
        current_bb_->AppendInst(inst);
        return inst;
    }

    template<typename... Args>
    void CreateNamedBlocks(std::map<std::string_view, BasicBlock*>& blocks, Args... names) {
        ( (
            blocks[names] = graph_->CreateNewBasicBlock(),
            std::cout << "Created block '" << names << "' with ID: " << blocks[names]->GetId() << std::endl 
        ), ... );
    }
};