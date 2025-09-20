#include "IRBuilder.hpp"
#include <iostream>
#include <cassert>
#include <algorithm>

std::unique_ptr<Graph> BuildFactorialGraph() {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());

    auto* entry_bb = graph->CreateNewBasicBlock();
    auto* loop_header_bb = graph->CreateNewBasicBlock();
    auto* loop_body_bb = graph->CreateNewBasicBlock();
    auto* exit_bb = graph->CreateNewBasicBlock();

    builder.SetInsertPoint(entry_bb);
    auto* n = builder.CreateParameter(Type::int32);
    auto* const_1 = builder.CreateConstant(Type::int32, 1);
    // cast uint32 to uint64
    builder.CreateJump(loop_header_bb);

    builder.SetInsertPoint(loop_header_bb);
    auto* result_phi = builder.CreatePhi(Type::int32);
    auto* i_phi = builder.CreatePhi(Type::int32);
    
    auto* cmp_res = builder.CreateCmp(i_phi, n);
    builder.CreateIf(cmp_res, loop_body_bb, exit_bb);

    builder.SetInsertPoint(loop_body_bb);
    auto* new_result = builder.CreateMul(result_phi, i_phi);
    auto* new_i = builder.CreateAdd(i_phi, const_1);
    builder.CreateJump(loop_header_bb);

    builder.SetInsertPoint(exit_bb);
    builder.CreateReturn(result_phi);

    result_phi->AddPhiInput(entry_bb, const_1);
    i_phi->AddPhiInput(entry_bb, const_1);
    result_phi->AddPhiInput(loop_body_bb, new_result);
    i_phi->AddPhiInput(loop_body_bb, new_i);
    
    return graph;
}

void TestFactorialGraph() {
    auto graph = BuildFactorialGraph();
    assert(graph != nullptr);
    assert(graph->GetBlocks().size() == 4);

    auto& blocks = graph->GetBlocks();
    auto it = blocks.begin();
    
    BasicBlock* entry_bb = (*it++).get();
    BasicBlock* loop_header_bb = (*it++).get();
    BasicBlock* loop_body_bb = (*it++).get();
    BasicBlock* exit_bb = (*it).get();

    assert(entry_bb->GetPreds().empty());
    assert(entry_bb->GetSuccs().size() == 1 && entry_bb->GetSuccs()[0] == loop_header_bb);
    assert(entry_bb->GetLastInst()->GetOpcode() == Opcode::Jump);

    assert(loop_header_bb->GetPreds().size() == 2); 
    assert(loop_header_bb->GetSuccs().size() == 2); 
    assert(loop_header_bb->GetLastInst()->GetOpcode() == Opcode::If);
    
    auto* first_inst = loop_header_bb->GetFirstInst();
    assert(first_inst->GetOpcode() == Opcode::Phi);
    auto* phi_result = static_cast<PhiInst*>(first_inst);
    assert(phi_result->GetInputs().size() == 2); 
    
    auto phi_inputs = phi_result->GetPhiInputs();
    bool from_entry_found = false;
    bool from_loop_body_found = false;
    for (const auto& pair : phi_inputs) {
        if (pair.first == entry_bb) {
            assert(pair.second->GetOpcode() == Opcode::Const);
            from_entry_found = true;
        } else if (pair.first == loop_body_bb) {
            assert(pair.second->GetOpcode() == Opcode::Mul);
            from_loop_body_found = true;
        }
    }
    assert(from_entry_found && from_loop_body_found);

    assert(loop_body_bb->GetPreds().size() == 1 && loop_body_bb->GetPreds()[0] == loop_header_bb);
    assert(loop_body_bb->GetSuccs().size() == 1 && loop_body_bb->GetSuccs()[0] == loop_header_bb);
    Instruction* current = loop_body_bb->GetFirstInst();
    assert(current->GetOpcode() == Opcode::Mul);
    current = current->GetNext();
    assert(current->GetOpcode() == Opcode::Add);
    current = current->GetNext();
    assert(current->GetOpcode() == Opcode::Jump);

    assert(exit_bb->GetPreds().size() == 1 && exit_bb->GetPreds()[0] == loop_header_bb);
    assert(exit_bb->GetSuccs().empty());
    assert(exit_bb->GetLastInst()->GetOpcode() == Opcode::Ret);

    assert(exit_bb->GetLastInst()->GetInputs()[0] == phi_result);
    
    std::cout << "Factorial graph test passed successfully!" << std::endl;
}

int main() {
    TestFactorialGraph();
    return 0;
}