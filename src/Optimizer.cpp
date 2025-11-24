#include "Optimizer.hpp"
#include <variant>


void Optimizer::Run() {
    bool changed = true;
    while (changed) {
        changed = false;
        for (auto& bb_ptr : graph_->GetBlocks()) {
            Instruction* inst = bb_ptr->GetFirstInst();
            while (inst) {
                Instruction* next = inst->GetNext();
                
                bool optimized = false;
                if (TryConstantFolding(inst)) {
                    optimized = true;
                } else if (TryPeephole(inst)) {
                    optimized = true;
                }

                if (optimized) {
                    changed = true;
                    bb_ptr->RemoveInst(inst);
                }
                
                inst = next;
            }
        }
    }
}

void Optimizer::ReplaceAllUses(Instruction* oldInst, Instruction* newInst) {
    for (auto& bb_ptr : graph_->GetBlocks()) {
        for (Instruction* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
            inst->ReplaceInput(oldInst, newInst);
        }
        for (Instruction* inst = bb_ptr->GetFirstPhi(); inst; inst = inst->GetNext()) {
            inst->ReplaceInput(oldInst, newInst);
        }
    }
}

bool Optimizer::TryConstantFolding(Instruction* inst) {
    auto* bin = dynamic_cast<BinaryInst*>(inst);
    if (!bin) return false;

    auto* c1 = dynamic_cast<ConstantInst*>(bin->GetInputs()[0]);
    auto* c2 = dynamic_cast<ConstantInst*>(bin->GetInputs()[1]);

    if (c1 && c2) {
        int64_t v1 = std::visit([](auto arg) { return static_cast<int64_t>(arg); }, c1->GetValue());
        int64_t v2 = std::visit([](auto arg) { return static_cast<int64_t>(arg); }, c2->GetValue());
        int64_t res = 0;

        switch (bin->GetOpcode()) {
            case Opcode::Mul: res = v1 * v2; break;
            case Opcode::Or:  res = v1 | v2; break;
            case Opcode::AShr: res = v1 >> v2; break;
            default: return false;
        }

        IRBuilder builder(graph_);
        builder.SetInsertPoint(inst->GetBasicBlock());
        auto* newConst = builder.CreateConstant(inst->GetType(), res);
        
        ReplaceAllUses(inst, newConst);
        return true;
    }
    return false;
}

bool Optimizer::TryPeephole(Instruction* inst) {
    auto* bin = dynamic_cast<BinaryInst*>(inst);
    if (!bin) return false;

    Instruction* lhs = bin->GetInputs()[0];
    Instruction* rhs = bin->GetInputs()[1];
    auto* const_rhs = dynamic_cast<ConstantInst*>(rhs);
    auto* const_lhs = dynamic_cast<ConstantInst*>(lhs);

    if (bin->GetOpcode() == Opcode::Mul && const_rhs) {
        int64_t val = std::visit([](auto arg) { return static_cast<int64_t>(arg); }, const_rhs->GetValue());
        
        if (val == 1) {
            ReplaceAllUses(inst, lhs);
            return true;
        }
        if (val == 0) {
            IRBuilder builder(graph_);
            builder.SetInsertPoint(inst->GetBasicBlock());
            auto* zero = builder.CreateConstant(inst->GetType(), 0);
            ReplaceAllUses(inst, zero);
            return true;
        }
    }

    if (bin->GetOpcode() == Opcode::Or) {
        if (lhs == rhs) {
            ReplaceAllUses(inst, lhs);
            return true;
        }
        if (const_rhs) {
            int64_t val = std::visit([](auto arg) { return static_cast<int64_t>(arg); }, const_rhs->GetValue());
            if (val == 0) {
                ReplaceAllUses(inst, lhs);
                return true;
            }
            if (val == -1) {
                ReplaceAllUses(inst, rhs);
                return true;
            }
        }
    }

    if (bin->GetOpcode() == Opcode::AShr) {
        // non commutative operation
        // x >> 0 = x
        // 0 >> x = 0
        if (const_rhs) {
            int64_t val = std::visit([](auto arg) { return static_cast<int64_t>(arg); }, const_rhs->GetValue());
            if (val == 0) {
                ReplaceAllUses(inst, lhs);
                return true;
            }
        }
        if (const_lhs) {
            int64_t val = std::visit([](auto arg) { return static_cast<int64_t>(arg); }, const_lhs->GetValue());
            if (val == 0) {
                ReplaceAllUses(inst, lhs);
                return true;
            }
        }
    }

    return false;
}