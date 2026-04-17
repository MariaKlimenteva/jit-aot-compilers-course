#pragma once
#include <vector>
#include <variant>
#include <iostream>

class BasicBlock;
class Graph;

enum class Opcode {
    Param, Const, Add, Mul, Cmp, Jump, If, Mov, Phi,
    BrCond, Br, Ret,
    Or, AShr, Call,
    NullCheck, BoundsCheck, LoadArray, StoreArray
};

enum class Type {
    Unknown, int32, int64
};

class Instruction {
protected:
    int id_;
    Opcode opcode_;
    Type type_;
    int life_position_ = -1;
    BasicBlock* basic_block_ = nullptr;
    Instruction* prev_ = nullptr;
    Instruction* next_ = nullptr;
    std::vector<Instruction*> inputs_;
    std::vector<Instruction*> users_;

public:
    Opcode GetOpcode() const { return opcode_; }
    Type GetType() const { return type_; }
    int GetId() const { return id_; }
    void SetLifePosition(int pos) { life_position_ = pos; }
    int GetLifePosition() const { return life_position_; }
    BasicBlock* GetBasicBlock() const { return basic_block_; }
    const std::vector<Instruction*>& GetInputs() const { return inputs_; }
    const std::vector<Instruction*>& GetUsers() const { return users_; }

    void AddInput(Instruction* input) {
        inputs_.push_back(input);
        if (input) {
            input->AddUser(this);
        }
    }

    void AddUser(Instruction* user) {
        users_.push_back(user);
    }

    void RemoveUser(Instruction* user) {
        for (auto it = users_.begin(); it != users_.end(); ++it) {
            if (*it == user) {
                users_.erase(it);
                break;
            }
        }
    }

    void SetBasicBlock(BasicBlock* bb) { basic_block_ = bb; }

    Instruction* GetNext() const { return next_; }
    Instruction* GetPrev() const { return prev_; }
    void SetNext(Instruction* next) { next_ = next; }
    void SetPrev(Instruction* prev) { prev_ = prev; }

    Instruction(int id, Opcode opcode, Type type, BasicBlock* bb)
        : id_(id), opcode_(opcode), type_(type), basic_block_(bb) {}
    virtual ~Instruction() = default;

    virtual void Dump() const = 0;

    virtual void ReplaceInput(Instruction* oldInst, Instruction* newInst) {
        for (auto& input : inputs_) {
            if (input == oldInst) {
                if (oldInst) oldInst->RemoveUser(this);
                input = newInst;
                if (newInst) newInst->AddUser(this);
            }
        }
    }
};

class BinaryInst : public Instruction {
public:
    BinaryInst(int id, Opcode opcode, Type type, BasicBlock* bb, Instruction* lhs, Instruction* rhs)
        : Instruction(id, opcode, type, bb) {
        AddInput(lhs);
        AddInput(rhs);
    }
    void Dump() const override; 
};

class ConstantInst : public Instruction {
public:
    using ValueType = std::variant<int32_t, int64_t>;

    ConstantInst(int id, Type type, BasicBlock* bb, ValueType value)
        : Instruction(id, Opcode::Const, type, bb), value_(value) {}

    ValueType GetValue() const { return value_; }
    void Dump() const override; 
private:
    ValueType value_;
};

class ReturnInst : public Instruction {
public:
    ReturnInst(int id, BasicBlock* bb, Instruction* value)
        : Instruction(id, Opcode::Ret, Type::Unknown, bb) {
        if (value) {
            AddInput(value);
        }
    }
    void Dump() const override; 
};

class ParameterInst : public Instruction {
public:
    ParameterInst(int id, Type type, BasicBlock* bb) 
        : Instruction(id, Opcode::Param, type, bb) {}
    
    void Dump() const override; 
};

class JumpInst : public Instruction {
public:
    JumpInst(int id, BasicBlock* bb, BasicBlock* target)
        : Instruction(id, Opcode::Jump, Type::Unknown, bb), target_(target) {}
    
    BasicBlock* GetTarget() const { return target_; }

    void Dump() const override; 
    void ReplaceTarget(BasicBlock* t) { target_ = t; }
private:
    BasicBlock* target_;
};

class IfInst : public Instruction {
public:
    IfInst(int id, BasicBlock* bb, Instruction* cond, BasicBlock* true_target, BasicBlock* false_target)
        : Instruction(id, Opcode::If, Type::Unknown, bb), true_target_(true_target), false_target_(false_target) {
        AddInput(cond);
    }

    BasicBlock* GetTrueTarget() const { return true_target_; }
    BasicBlock* GetFalseTarget() const { return false_target_; }

    void Dump() const override;
    void ReplaceTargets(BasicBlock* t, BasicBlock* f) { true_target_ = t; false_target_ = f; }
private:
    BasicBlock* true_target_;
    BasicBlock* false_target_;
};

class PhiInst : public Instruction {
public:
    PhiInst(int id, Type type, BasicBlock* bb)
        : Instruction(id, Opcode::Phi, type, bb) {}
    
    void AddPhiInput(BasicBlock* from, Instruction* value) {
        phi_inputs_.emplace_back(from, value);
        AddInput(value);
    }

    const std::vector<std::pair<BasicBlock*, Instruction*>>& GetPhiInputs() const {
        return phi_inputs_;
    }

    void Dump() const override; 

    void ReplaceInput(Instruction* oldInst, Instruction* newInst) override {
        Instruction::ReplaceInput(oldInst, newInst);
        for (auto& pair : phi_inputs_) {
            if (pair.second == oldInst) pair.second = newInst;
        }
    }
    void ReplaceBlock(BasicBlock* old_bb, BasicBlock* new_bb) {
        for (auto& pair : phi_inputs_) {
            if (pair.first == old_bb) pair.first = new_bb;
        }
    }

private:
    std::vector<std::pair<BasicBlock*, Instruction*>> phi_inputs_;
};

class NullCheckInst : public Instruction {
public:
    NullCheckInst(int id, Type type, BasicBlock* bb, Instruction* obj)
        : Instruction(id, Opcode::NullCheck, type, bb) {
        AddInput(obj);
    }
    Instruction* GetCheckedObject() const { return GetInputs()[0]; }
    void Dump() const override;
};

class BoundsCheckInst : public Instruction {
public:
    BoundsCheckInst(int id, Type type, BasicBlock* bb,
                    Instruction* index, Instruction* length)
        : Instruction(id, Opcode::BoundsCheck, type, bb) {
        AddInput(index);
        AddInput(length);
    }
    Instruction* GetIndex() const { return GetInputs()[0]; }
    Instruction* GetLength() const { return GetInputs()[1]; }
    void Dump() const override;
};

class LoadArrayInst : public Instruction {
public:
    LoadArrayInst(int id, Type type, BasicBlock* bb,
                  Instruction* arr, Instruction* index)
        : Instruction(id, Opcode::LoadArray, type, bb) {
        AddInput(arr);
        AddInput(index);
    }
    Instruction* GetArray() const { return GetInputs()[0]; }
    Instruction* GetIndex() const { return GetInputs()[1]; }
    void Dump() const override;
};

class StoreArrayInst : public Instruction {
public:
    StoreArrayInst(int id, Type type, BasicBlock* bb,
                   Instruction* arr, Instruction* index, Instruction* value)
        : Instruction(id, Opcode::StoreArray, type, bb) {
        AddInput(arr);
        AddInput(index);
        AddInput(value);
    }
    Instruction* GetArray() const { return GetInputs()[0]; }
    Instruction* GetIndex() const { return GetInputs()[1]; }
    Instruction* GetValue() const { return GetInputs()[2]; }
    void Dump() const override;
};

class CallInst : public Instruction {
public:
    CallInst(int id, Type type, BasicBlock* bb, Graph* callee, const std::vector<Instruction*>& args)
        : Instruction(id, Opcode::Call, type, bb), callee_(callee) {
        for (auto* arg : args) {
            AddInput(arg);
        }
    }
    
    Graph* GetCallee() const { return callee_; }
    
    void Dump() const override {
        std::cout << "v" << GetId() << " = call Graph_" << callee_ << "(";
        for (size_t i = 0; i < GetInputs().size(); ++i) {
            std::cout << "v" << GetInputs()[i]->GetId() << (i < GetInputs().size() - 1 ? ", " : "");
        }
        std::cout << ")" << std::endl;
    }
private:
    Graph* callee_;
};