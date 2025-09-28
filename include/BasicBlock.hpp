#pragma once
#include <vector>
#include <string>
#include "Instruction.hpp"
#include "termcolor.hpp"

class Graph;

class BasicBlock {
private:
    Graph* graph_;
    std::vector<BasicBlock*> preds_;
    std::vector<BasicBlock*> succs_;

    Instruction* first_phi_ = nullptr;
    Instruction* last_phi_ = nullptr;
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
    void Dump() const {
        std::cout << "BB" << std::endl;
        for (auto* cur_i = first_inst_; cur_i != nullptr; cur_i = cur_i->GetNext()) {
            cur_i->Dump();
            if (cur_i == last_inst_) {
                break;
            }
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
        inst->SetBasicBlock(this);
        inst->SetNext(nullptr);
        inst->SetPrev(nullptr);
        if (inst->GetOpcode() == Opcode::Phi) {
            if (first_phi_ == nullptr) {
                first_phi_ = inst;
                last_phi_ = inst;
                if (first_inst_ == nullptr) {
                    first_inst_ = inst;
                    last_inst_ = inst;
                }
            } else {
                last_phi_->SetNext(inst);
                inst->SetPrev(last_phi_);
                last_phi_ = inst;
            }
        } else {
            if (first_inst_ == nullptr) {
                first_inst_ = inst;
                last_inst_ = inst;
            } else {
                last_inst_->SetNext(inst);
                inst->SetPrev(last_inst_);
                last_inst_ = inst;
            }      
        }
    }
};