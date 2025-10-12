#pragma once

#include <algorithm>
#include <map>
  
std::unique_ptr<Graph> BuildFactorialGraph() {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());

    auto* entry_bb = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry_bb);
    auto* loop_header_bb = graph->CreateNewBasicBlock();
    auto* loop_body_bb = graph->CreateNewBasicBlock();
    auto* exit_bb = graph->CreateNewBasicBlock();

    builder.SetInsertPoint(entry_bb);
    auto* n = builder.CreateParameter(Type::int32);
    auto* const_1 = builder.CreateConstant(Type::int64, 1);
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

std::unique_ptr<Graph> BuildExample1Graph(std::map<std::string_view, BasicBlock*>& blocks) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    builder.CreateNamedBlocks(blocks, "A", "B", "C", "D", "E", "F", "G");

    graph->SetEntryBlock(blocks["A"]);
    builder.SetInsertPoint(blocks["A"]);

    blocks["A"]->LinkTo(blocks["B"]);
    blocks["B"]->LinkTo(blocks["C"], blocks["F"]);
    blocks["F"]->LinkTo(blocks["E"], blocks["G"]);
    blocks["C"]->LinkTo(blocks["D"]);
    blocks["G"]->LinkTo(blocks["D"]);
    blocks["E"]->LinkTo(blocks["D"]);
    return graph;
}

std::unique_ptr<Graph> BuildExample2Graph(std::map<std::string_view, BasicBlock*>& blocks) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    builder.CreateNamedBlocks(blocks, "A", "B", "C", "D", "E", "F", "G", "I", "H", "J", "K");

    graph->SetEntryBlock(blocks["A"]);
    builder.SetInsertPoint(blocks["A"]);

    blocks["A"]->LinkTo(blocks["B"]);
    blocks["B"]->LinkTo(blocks["C"], blocks["J"]);
    blocks["C"]->LinkTo(blocks["D"]);
    blocks["J"]->LinkTo(blocks["C"]);
    blocks["D"]->LinkTo(blocks["E"], blocks["C"]);
    blocks["E"]->LinkTo(blocks["F"]);
    blocks["F"]->LinkTo(blocks["G"], blocks["E"]);
    blocks["G"]->LinkTo(blocks["H"], blocks["I"]);
    blocks["H"]->LinkTo(blocks["B"]);
    blocks["I"]->LinkTo(blocks["K"]);
    return graph;
}

std::unique_ptr<Graph> BuildExample3Graph(std::map<std::string_view, BasicBlock*>& blocks) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    builder.CreateNamedBlocks(blocks, "A", "B", "C", "D", "E", "F", "G", "H", "I");

    graph->SetEntryBlock(blocks["A"]);
    builder.SetInsertPoint(blocks["A"]);

    blocks["A"]->LinkTo(blocks["B"]);
    blocks["B"]->LinkTo(blocks["C"], blocks["E"]);
    blocks["C"]->LinkTo(blocks["D"]);
    blocks["D"]->LinkTo(blocks["G"]);
    blocks["E"]->LinkTo(blocks["F"], blocks["D"]);
    blocks["F"]->LinkTo(blocks["H"], blocks["B"]);
    blocks["G"]->LinkTo(blocks["C"], blocks["I"]);
    blocks["H"]->LinkTo(blocks["I"], blocks["G"]);
    return graph;
}