#include "CheckElimination.hpp"
#include "DominatorAnalysis.hpp"
#include "LoopAnalyzer.hpp"
#include "LinearOrderBuilder.hpp"
#include "LivenessAnalysis.hpp"
#include "IRBuilder.hpp"
#include "TestRunner.hpp"
#include "TestsUtils.hpp"

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
    auto* load1 = builder.CreateLoadArray(Type::int32, nc1, bc1);
    (void)load1;
    builder.CreateJump(bb_b);

    builder.SetInsertPoint(bb_b);
    auto* nc2 = builder.CreateNullCheck(arr);
    auto* bc2 = builder.CreateBoundsCheck(idx, len);
    auto* load2 = builder.CreateLoadArray(Type::int32, nc2, bc2);
    builder.CreateReturn(load2);

    entry->LinkTo(bb_a);
    bb_a->LinkTo(bb_b);

    std::cout << "\n--- BEFORE CheckElimination ---" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    LoopAnalyzer loops(graph.get(), &dom);
    loops.Run();

    CheckElimination ce(graph.get(), &dom, &loops);
    ce.Run();

    std::cout << "\n--- AFTER CheckElimination (removed=" << ce.GetRemovedCount()
              << ", hoisted=" << ce.GetHoistedCount() << ") ---" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 2);
    ASSERT_EQ(ce.GetHoistedCount(), 0);

    bool nc1_alive = false, bc1_alive = false;
    bool nc2_alive = false, bc2_alive = false;
    for (auto& bb_ptr : graph->GetBlocks()) {
        for (auto* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (inst == nc1) nc1_alive = true;
            if (inst == bc1) bc1_alive = true;
            if (inst == nc2) nc2_alive = true;
            if (inst == bc2) bc2_alive = true;
        }
    }
    ASSERT_EQ(nc1_alive, true);
    ASSERT_EQ(bc1_alive, true);
    ASSERT_EQ(nc2_alive, false);
    ASSERT_EQ(bc2_alive, false);

    std::cout << "NullCheck redundant test passed!" << std::endl;
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
    auto* load1 = builder.CreateLoadArray(Type::int32, nc1, bc1);
    auto* nc2 = builder.CreateNullCheck(arr2);
    auto* bc2 = builder.CreateBoundsCheck(idx, len);
    auto* load2 = builder.CreateLoadArray(Type::int32, nc2, bc2);
    auto* sum = builder.CreateAdd(load1, load2);
    builder.CreateReturn(sum);

    entry->LinkTo(bb_a);

    std::cout << "\n--- BEFORE CheckElimination ---" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    LoopAnalyzer loops(graph.get(), &dom);
    loops.Run();

    CheckElimination ce(graph.get(), &dom, &loops);
    ce.Run();

    std::cout << "\n--- AFTER CheckElimination (removed=" << ce.GetRemovedCount()
              << ", hoisted=" << ce.GetHoistedCount() << ") ---" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 1);
    ASSERT_EQ(ce.GetHoistedCount(), 0);

    bool nc1_alive = false, nc2_alive = false;
    bool bc1_alive = false, bc2_alive = false;
    for (auto& bb_ptr : graph->GetBlocks()) {
        for (auto* inst = bb_ptr->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (inst == nc1) nc1_alive = true;
            if (inst == nc2) nc2_alive = true;
            if (inst == bc1) bc1_alive = true;
            if (inst == bc2) bc2_alive = true;
        }
    }
    ASSERT_EQ(nc1_alive, true);
    ASSERT_EQ(nc2_alive, true);
    ASSERT_EQ(bc1_alive, true);
    ASSERT_EQ(bc2_alive, false);

    std::cout << "NullCheck no-remove different values test passed!" << std::endl;
}

void TestCheckHoistFromLoop(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());

    auto* entry = graph->CreateNewBasicBlock();
    auto* header = graph->CreateNewBasicBlock();
    auto* body = graph->CreateNewBasicBlock();
    auto* exit = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);

    builder.SetInsertPoint(entry);
    auto* arr = builder.CreateParameter(Type::int32);
    auto* len = builder.CreateConstant(Type::int32, 10);
    auto* i_init = builder.CreateConstant(Type::int32, 0);
    auto* one = builder.CreateConstant(Type::int32, 1);
    builder.CreateJump(header);

    builder.SetInsertPoint(header);
    auto* i_phi = builder.CreatePhi(Type::int32);
    auto* cmp = builder.CreateCmp(i_phi, len);
    builder.CreateIf(cmp, body, exit);

    builder.SetInsertPoint(body);
    auto* nc = builder.CreateNullCheck(arr);
    auto* bc = builder.CreateBoundsCheck(i_phi, len);
    auto* load = builder.CreateLoadArray(Type::int32, nc, bc);
    (void)load;
    auto* i_new = builder.CreateAdd(i_phi, one);
    builder.CreateJump(header);

    builder.SetInsertPoint(exit);
    builder.CreateReturn(i_phi);

    i_phi->AddPhiInput(entry, i_init);
    i_phi->AddPhiInput(body, i_new);

    std::cout << "\n--- BEFORE CheckElimination ---" << std::endl;
    graph->Dump();

    DominatorAnalysis dom(graph.get());
    dom.Run();
    LoopAnalyzer loops_analyzer(graph.get(), &dom);
    loops_analyzer.Run();

    const auto& all_loops = loops_analyzer.GetLoops();
    ASSERT_EQ(all_loops.size(), static_cast<size_t>(1));
    Loop* loop = all_loops[0].get();
    ASSERT_EQ(loop->Contains(body), true);
    ASSERT_EQ(loop->Contains(entry), false);

    CheckElimination ce(graph.get(), &dom, &loops_analyzer);
    ce.Run();

    std::cout << "\n--- AFTER CheckElimination (removed=" << ce.GetRemovedCount()
              << ", hoisted=" << ce.GetHoistedCount() << ") ---" << std::endl;
    graph->Dump();

    ASSERT_EQ(ce.GetRemovedCount(), 0);
    ASSERT_EQ(ce.GetHoistedCount(), 1);

    bool nc_in_body = false, bc_in_body = false;
    for (auto* inst = body->GetFirstInst(); inst; inst = inst->GetNext()) {
        if (inst == nc) nc_in_body = true;
        if (inst == bc) bc_in_body = true;
    }
    ASSERT_EQ(nc_in_body, false);
    ASSERT_EQ(bc_in_body, true);

    std::cout << "Check hoist from loop test passed!" << std::endl;
}
