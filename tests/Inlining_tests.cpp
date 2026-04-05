#include "TestRunner.hpp"
#include "IRBuilder.hpp"
#include "TestsUtils.hpp"
#include "Inliner.hpp"
#include <map>

std::unique_ptr<Graph> BuildCalleeGraph() {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    std::map<std::string_view, BasicBlock*> blocks;
    builder.CreateNamedBlocks(blocks, "Block2_Start", "Block3", "Block4", "Block5");

    graph->SetEntryBlock(blocks["Block2_Start"]);

    // Block 2
    builder.SetInsertPoint(blocks["Block2_Start"]);
    auto* param1 = builder.CreateParameter(Type::int32);
    auto* param2 = builder.CreateParameter(Type::int32);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    auto* c10 = builder.CreateConstant(Type::int32, 10);
    builder.CreateJump(blocks["Block3"]);

    // Block 3
    builder.SetInsertPoint(blocks["Block3"]);
    auto* use1 = builder.CreateAdd(param1, c1);
    auto* use2 = builder.CreateCmp(param2, c10);
    builder.CreateIf(use2, blocks["Block4"], blocks["Block5"]);

    // Block 4 (Return 1)
    builder.SetInsertPoint(blocks["Block4"]);
    auto* def3 = builder.CreateAdd(use1, c1);
    builder.CreateReturn(def3);

    // Block 5 (Return 2)
    builder.SetInsertPoint(blocks["Block5"]);
    auto* def4 = builder.CreateMul(use1, c10);
    builder.CreateReturn(def4);

    return graph;
}

void TestInliningSlideExample(TestRunner& t) {
    auto callee = BuildCalleeGraph();
    
    auto caller = std::make_unique<Graph>();
    IRBuilder builder(caller.get());
    std::map<std::string_view, BasicBlock*> blocks;
    builder.CreateNamedBlocks(blocks, "Block0_Start", "Block1", "Block6_End");

    caller->SetEntryBlock(blocks["Block0_Start"]);

    // Block 0
    builder.SetInsertPoint(blocks["Block0_Start"]);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    auto* c5 = builder.CreateConstant(Type::int32, 5);
    builder.CreateJump(blocks["Block1"]);

    // Block 1
    builder.SetInsertPoint(blocks["Block1"]);
    auto* def1 = builder.CreateAdd(c1, c5);
    auto* def2 = builder.CreateMul(c5, c5);
    
    // Call
    auto* call = builder.CreateCall(Type::int32, callee.get(), {def1, def2});

    auto* use_after_call = builder.CreateAdd(call, def1);
    builder.CreateReturn(use_after_call);

    std::cout << "\n=== BEFORE INLINING ===" << std::endl;
    caller->Dump();

    Inliner inliner(caller.get());
    inliner.Run();

    std::cout << "\n=== AFTER INLINING ===" << std::endl;
    caller->Dump();

    bool has_call = false;
    for (const auto& bb : caller->GetBlocks()) {
        for (auto* inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (inst->GetOpcode() == Opcode::Call) has_call = true;
        }
    }
    ASSERT_EQ(has_call, false);
    ASSERT_NOT_EQ(caller->GetBlocks().size(), static_cast<size_t>(3));
}