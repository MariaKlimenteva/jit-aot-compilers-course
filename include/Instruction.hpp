#pragma once
#include <vector>
#include <variant>

class BasicBlock;

// IR set
enum class Opcode {
    Param, Const, Add, Mul, Cmp, Jump, If, Mov, Phi,
    BrCond, Br, Ret
};


enum class Type {
    Unknown, int32, int64
};

class Instruction {
protected:
    Opcode opcode_;
    Type type_;
    BasicBlock* basic_block_ = nullptr;
    Instruction* prev_ = nullptr;
    Instruction* next_ = nullptr;
    std::vector<Instruction*> inputs_;

public:
    Opcode GetOpcode() const { return opcode_; }
    Type GetType() const { return type_; }
    BasicBlock* GetBasicBlock() const { return basic_block_; }
    const std::vector<Instruction*>& GetInputs() const { return inputs_; }

    void AddInput(Instruction* input) { inputs_.push_back(input); }
    void SetBasicBlock(BasicBlock* bb) { basic_block_ = bb; }

    Instruction* GetNext() const { return next_; }
    Instruction* GetPrev() const { return prev_; }
    void SetNext(Instruction* next) { next_ = next; }
    void SetPrev(Instruction* prev) { prev_ = prev; }

    Instruction(Opcode opcode, Type type, BasicBlock* bb)
        : opcode_(opcode), type_(type), basic_block_(bb) {}
    virtual ~Instruction() = default;
};

// ADD, MUL, CMP
class BinaryInst : public Instruction {
public:
    BinaryInst(Opcode opcode, Type type, BasicBlock* bb, Instruction* lhs, Instruction* rhs)
        : Instruction(opcode, type, bb) {
        AddInput(lhs);
        AddInput(rhs);
    }
};

class ConstantInst : public Instruction {
public:
    using ValueType = std::variant<int32_t, int64_t>;

    ConstantInst(Type type, BasicBlock* bb, ValueType value)
        : Instruction(Opcode::Const, type, bb), value_(value) {}

    ValueType GetValue() const { return value_; }

private:
    ValueType value_;
};

class ReturnInst : public Instruction {
public:
    ReturnInst(BasicBlock* bb, Instruction* value)
        : Instruction(Opcode::Ret, Type::Unknown, bb) {
        if (value) {
            AddInput(value);
        }
    }
};

class ParameterInst : public Instruction {
public:
    ParameterInst(Type type, BasicBlock* bb) 
        : Instruction(Opcode::Param, type, bb) {}
};

class JumpInst : public Instruction {
public:
    JumpInst(BasicBlock* bb, BasicBlock* target)
        : Instruction(Opcode::Jump, Type::Unknown, bb), target_(target) {}
    
    BasicBlock* GetTarget() const { return target_; }
private:
    BasicBlock* target_;
};

class IfInst : public Instruction {
public:
    IfInst(BasicBlock* bb, Instruction* cond, BasicBlock* true_target, BasicBlock* false_target)
        : Instruction(Opcode::If, Type::Unknown, bb), true_target_(true_target), false_target_(false_target) {
        AddInput(cond);
    }

    BasicBlock* GetTrueTarget() const { return true_target_; }
    BasicBlock* GetFalseTarget() const { return false_target_; }

private:
    BasicBlock* true_target_;
    BasicBlock* false_target_;
};

class PhiInst : public Instruction {
public:
    PhiInst(Type type, BasicBlock* bb)
        : Instruction(Opcode::Phi, type, bb) {}
    
    void AddPhiInput(BasicBlock* from, Instruction* value) {
        phi_inputs_.emplace_back(from, value);
        AddInput(value);
    }

    const std::vector<std::pair<BasicBlock*, Instruction*>>& GetPhiInputs() const {
        return phi_inputs_;
    }

private:
    std::vector<std::pair<BasicBlock*, Instruction*>> phi_inputs_;
};