#include "CheckElimination.hpp"
#include "DominatorAnalysis.hpp"
#include "IRBuilder.hpp"
#include "TestRunner.hpp"
#include "TestsUtils.hpp"

static bool IsInstructionAlive(Graph* graph, Instruction* target) {
    for (auto& bb : graph->GetBlocks()) {
        for (auto* inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (inst == target) return true;
        }
    }
    return false;
}

void TestNullCheckRedundant(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* entry = graph->CreateNewBasicBlock();
    auto* bb_a = graph->CreateNewBasicBlock();
    auto* bb_b = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);
    builder.SetInsertPoint(entry);
    auto* arr = builder.CreateParameter(Type::int32);
    auto* idx = builder.CreateParameter(Type::int32);
    auto* len = builder.CreateConstant(Type::int32, 10);
    builder.CreateJump(bb_a);
    builder.SetInsertPoint(bb_a);
    auto* nc1 = builder.CreateNullCheck(arr);
    auto* bc1 = builder.CreateBoundsCheck(idx, len);
    builder.CreateLoadArray(Type::int32, nc1, bc1);
    builder.CreateJump(bb_b);
    builder.SetInsertPoint(bb_b);
    auto* nc2 = builder.CreateNullCheck(arr);
    auto* bc2 = builder.CreateBoundsCheck(idx, len);
    auto* load2 = builder.CreateLoadArray(Type::int32, nc2, bc2);
    builder.CreateReturn(load2);
    entry->LinkTo(bb_a);
    bb_a->LinkTo(bb_b);

    std::cout << "TestNullCheckRedundant:\nBEFORE:" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    CheckElimination ce(graph.get(), &dom);
    ce.Run();

    std::cout << "AFTER (Removed: " << ce.GetRemovedCount() << "):" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 2);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), bc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc2), false);
    ASSERT_EQ(IsInstructionAlive(graph.get(), bc2), false);
    ASSERT_EQ(load2->GetInputs()[0], nc1);
    ASSERT_EQ(load2->GetInputs()[1], bc1);
}

void TestNullCheckNoRemoveDifferentValues(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* entry = graph->CreateNewBasicBlock();
    auto* bb_a = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);
    builder.SetInsertPoint(entry);
    auto* arr1 = builder.CreateParameter(Type::int32);
    auto* arr2 = builder.CreateParameter(Type::int32);
    auto* idx = builder.CreateParameter(Type::int32);
    auto* len = builder.CreateConstant(Type::int32, 10);
    builder.CreateJump(bb_a);
    builder.SetInsertPoint(bb_a);
    auto* nc1 = builder.CreateNullCheck(arr1);
    auto* bc1 = builder.CreateBoundsCheck(idx, len);
    auto* l1 = builder.CreateLoadArray(Type::int32, nc1, bc1);
    auto* nc2 = builder.CreateNullCheck(arr2);
    auto* bc2 = builder.CreateBoundsCheck(idx, len);
    auto* l2 = builder.CreateLoadArray(Type::int32, nc2, bc2);
    builder.CreateReturn(builder.CreateAdd(l1, l2));
    entry->LinkTo(bb_a);

    std::cout << "TestNullCheckNoRemoveDifferentValues:\nBEFORE:" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    CheckElimination ce(graph.get(), &dom);
    ce.Run();

    std::cout << "AFTER (Removed: " << ce.GetRemovedCount() << "):" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 1); 
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc2), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), bc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), bc2), false);
    ASSERT_EQ(l2->GetInputs()[1], bc1);
}

void TestChecksInSameBlock(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* entry = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);
    builder.SetInsertPoint(entry);
    auto* ptr = builder.CreateParameter(Type::int64);
    auto* nc1 = builder.CreateNullCheck(ptr);
    auto* nc2 = builder.CreateNullCheck(ptr);
    auto* nc3 = builder.CreateNullCheck(ptr);
    auto* ret = builder.CreateReturn(nc3);

    std::cout << "TestChecksInSameBlock:\nBEFORE:" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    CheckElimination ce(graph.get(), &dom);
    ce.Run();

    std::cout << "AFTER (Removed: " << ce.GetRemovedCount() << "):" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 2);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc2), false);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc3), false);
    ASSERT_EQ(ret->GetInputs()[0], nc1);
}

void TestChecksDiamondNoElimination(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* entry = graph->CreateNewBasicBlock();
    auto* left = graph->CreateNewBasicBlock();
    auto* right = graph->CreateNewBasicBlock();
    auto* exit = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);
    builder.SetInsertPoint(entry);
    auto* ptr = builder.CreateParameter(Type::int64);
    auto* cond = builder.CreateParameter(Type::int32);
    builder.CreateIf(cond, left, right);
    builder.SetInsertPoint(left);
    auto* nc1 = builder.CreateNullCheck(ptr);
    builder.CreateJump(exit);
    builder.SetInsertPoint(right);
    auto* nc2 = builder.CreateNullCheck(ptr); 
    builder.CreateJump(exit);
    builder.SetInsertPoint(exit);
    builder.CreateReturn(nullptr);
    entry->LinkTo(left, right);
    left->LinkTo(exit);
    right->LinkTo(exit);

    std::cout << "TestChecksDiamondNoElimination:\nBEFORE:" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    CheckElimination ce(graph.get(), &dom);
    ce.Run();

    std::cout << "AFTER (Removed: " << ce.GetRemovedCount() << "):" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 0);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), nc2), true);
}

void TestBoundsCheckDifferentLength(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* entry = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);
    builder.SetInsertPoint(entry);
    auto* idx = builder.CreateParameter(Type::int32);
    auto* len1 = builder.CreateConstant(Type::int32, 10);
    auto* len2 = builder.CreateConstant(Type::int32, 20);
    auto* bc1 = builder.CreateBoundsCheck(idx, len1);
    auto* bc2 = builder.CreateBoundsCheck(idx, len2);
    builder.CreateReturn(nullptr);

    std::cout << "TestBoundsCheckDifferentLength:\nBEFORE:" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    CheckElimination ce(graph.get(), &dom);
    ce.Run();

    std::cout << "AFTER (Removed: " << ce.GetRemovedCount() << "):" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 0);
    ASSERT_EQ(IsInstructionAlive(graph.get(), bc1), true);
    ASSERT_EQ(IsInstructionAlive(graph.get(), bc2), true);
}
