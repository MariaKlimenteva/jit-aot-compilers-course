#include "Instruction.hpp"
#include "BasicBlock.hpp"

static std::string TypeToString(Type type) {
    switch (type) {
        case Type::int32: return ".i32";
        case Type::int64: return ".i64";
        default: return "";
    }
}

static std::string OpcodeToString(Opcode opcode) {
    switch (opcode) {
        case Opcode::Add: return "add";
        case Opcode::Mul: return "mul";
        case Opcode::Cmp: return "cmp";
        case Opcode::Jump: return "jump";
        case Opcode::If: return "if";
        case Opcode::Phi: return "phi";
        case Opcode::Ret: return "ret";
        case Opcode::Const: return "const";
        case Opcode::Param: return "param";
        default: return "unknown";
    }
}

void BinaryInst::Dump() const {
    std::cout << "v" << GetId() << TypeToString(GetType()) << " = "
              << OpcodeToString(GetOpcode()) << " v" << GetInputs()[0]->GetId()
              << ", v" << GetInputs()[1]->GetId() << std::endl;
}

void ConstantInst::Dump() const {
    std::cout << "v" << GetId() << TypeToString(GetType()) << " = "
              << OpcodeToString(GetOpcode()) << " ";
    std::visit([](auto&& arg) {
        std::cout << arg;
    }, value_);
    std::cout << std::endl;
}

void JumpInst::Dump() const {
    std::cout << OpcodeToString(GetOpcode()) << " BB<" << target_->GetId() << ">" << std::endl;
}

void IfInst::Dump() const {
    std::cout << OpcodeToString(GetOpcode()) << " v" << GetInputs()[0]->GetId()
              << ", BB<" << GetTrueTarget()->GetId()
              << ">, BB<" << GetFalseTarget()->GetId() << ">" << std::endl;
}

void ParameterInst::Dump() const {
    std::cout << "v" << GetId() << TypeToString(GetType())
              << " = " << OpcodeToString(GetOpcode()) << std::endl;
}

void ReturnInst::Dump() const {
    std::cout << OpcodeToString(GetOpcode());
    if (!GetInputs().empty()) {
        std::cout << " v" << GetInputs()[0]->GetId();
    }
    std::cout << std::endl;
}

void PhiInst::Dump() const {
    std::cout << "v" << GetId() << TypeToString(GetType()) << " = "
              << OpcodeToString(GetOpcode()) << " ";
    for (size_t i = 0; i < phi_inputs_.size(); ++i) {
        auto& [from, value] = phi_inputs_[i];
        std::cout << "[ BB<" << from->GetId() << ">, v" << value->GetId() << " ]";
        if (i < phi_inputs_.size() - 1) {
            std::cout << ", ";
        }
    }
    std::cout << std::endl;
}