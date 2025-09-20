#pragma once
#include <vector>
#include <string>
#include "Instruction.hpp"

class Graph;

class BasicBlock {
private:
    Graph* graph_;
    std::vector<BasicBlock*> preds_;
    std::vector<BasicBlock*> succs_;

    Instruction* first_phi_ = nullptr;
    Instruction* first_inst_ = nullptr;
    Instruction* last_inst_ = nullptr;
public:
    explicit BasicBlock(Graph* graph) : graph_(graph) {}
    ~BasicBlock() {
        Instruction* current = first_phi_;
        while (current) {
            Instruction* next = current->GetNext();
            delete current;
            current = next;
        }
    }

    void AddPred(BasicBlock* pred) { preds_.push_back(pred); }
    void AddSucc(BasicBlock* succ) { succs_.push_back(succ); }

    const std::vector<BasicBlock*>& GetPreds() const { return preds_; }
    const std::vector<BasicBlock*>& GetSuccs() const { return succs_; }
    Graph* GetGraph() const { return graph_; }
    Instruction* GetFirstPhi() const { return first_phi_; }
    Instruction* GetFirstInst() const { return first_inst_; }
    Instruction* GetLastInst() const { return last_inst_; }

    void AppendInst(Instruction* inst) {
        if (inst->GetOpcode() == Opcode::Phi) {
            if (!first_phi_) {
                first_phi_ = inst;
            }

            if (first_inst_) {
                inst->SetNext(first_inst_);
                first_inst_->SetPrev(inst);
            }
        }

        if (!first_inst_) {
            first_inst_ = inst;
        }

        if (last_inst_) {
            last_inst_->SetNext(inst);
            inst->SetPrev(last_inst_);
        }
        last_inst_ = inst;
    }
};