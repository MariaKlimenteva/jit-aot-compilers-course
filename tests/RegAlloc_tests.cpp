
#include "LinearOrderBuilder.hpp"
#include "RegisterAllocator.hpp"
#include "TestRunner.hpp"
#include "IRBuilder.hpp"

// ============================================================================
// Test 1: If-Else
// ============================================================================
/* 
 *      [Entry]
 *         |
 *         v
 *   [LoopHeader] <--------+
 *     |       |           |
 * (exit)      v           |
 *  [Exit]  [LoopBody]     |
 *           |      |      |
 *           v      v      |
 *      [IfTrue] [IfFalse] |
 *           |      |      |
 *           v      v      |
 *         [LoopLatch] ----+
 */
inline std::unique_ptr<Graph> BuildRegAllocGraph1(std::map<std::string_view, BasicBlock*>& blocks) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    builder.CreateNamedBlocks(blocks, "Entry", "LoopHeader", "LoopBody", "IfTrue", "IfFalse", "LoopLatch", "Exit");

    graph->SetEntryBlock(blocks["Entry"]);
    
    // Entry
    builder.SetInsertPoint(blocks["Entry"]);
    auto* param = builder.CreateParameter(Type::int32);
    auto* c100 = builder.CreateConstant(Type::int32, 100);
    auto* c10 = builder.CreateConstant(Type::int32, 10);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    builder.CreateJump(blocks["LoopHeader"]);

    // LoopHeader
    builder.SetInsertPoint(blocks["LoopHeader"]);
    auto* i_phi = builder.CreatePhi(Type::int32);
    auto* cond_exit = builder.CreateCmp(i_phi, c100);
    builder.CreateIf(cond_exit, blocks["Exit"], blocks["LoopBody"]);

    // LoopBody
    builder.SetInsertPoint(blocks["LoopBody"]);
    auto* cond_if = builder.CreateCmp(i_phi, c10);
    builder.CreateIf(cond_if, blocks["IfTrue"], blocks["IfFalse"]);

    // IfTrue
    builder.SetInsertPoint(blocks["IfTrue"]);
    auto* add_true = builder.CreateAdd(i_phi, c1);
    builder.CreateJump(blocks["LoopLatch"]);

    // IfFalse
    builder.SetInsertPoint(blocks["IfFalse"]);
    auto* add_false = builder.CreateAdd(i_phi, c10);
    builder.CreateJump(blocks["LoopLatch"]);

    // LoopLatch
    builder.SetInsertPoint(blocks["LoopLatch"]);
    auto* merge_phi = builder.CreatePhi(Type::int32);
    merge_phi->AddPhiInput(blocks["IfTrue"], add_true);
    merge_phi->AddPhiInput(blocks["IfFalse"], add_false);
    builder.CreateJump(blocks["LoopHeader"]);

    // Phi
    i_phi->AddPhiInput(blocks["Entry"], param);
    i_phi->AddPhiInput(blocks["LoopLatch"], merge_phi);

    // Exit
    builder.SetInsertPoint(blocks["Exit"]);
    builder.CreateReturn(i_phi);

    return graph;
}

// ============================================================================
// Test 2: Nested Loops
// ============================================================================
/*
 *      [Entry]
 *         |
 *         v
 *   [OuterHeader] <----------+
 *     |        |             |
 *   (exit)     v             |
 *   [Exit] [InnerHeader] <-+ |
 *              |      |    | |
 *         (out_latch) v    | |
 *              |  [InnerB]-+ |
 *              v             |
 *         [OuterLatch] ------+
 */
inline std::unique_ptr<Graph> BuildRegAllocGraph2(std::map<std::string_view, BasicBlock*>& blocks) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    builder.CreateNamedBlocks(blocks, "Entry", "OuterHeader", "InnerHeader", "InnerBody", "OuterLatch", "Exit");

    graph->SetEntryBlock(blocks["Entry"]);

    builder.SetInsertPoint(blocks["Entry"]);
    auto* p = builder.CreateParameter(Type::int32);
    auto* c10 = builder.CreateConstant(Type::int32, 10);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    builder.CreateJump(blocks["OuterHeader"]);

    builder.SetInsertPoint(blocks["OuterHeader"]);
    auto* i_phi = builder.CreatePhi(Type::int32);
    auto* cmp_out = builder.CreateCmp(i_phi, c10);
    builder.CreateIf(cmp_out, blocks["Exit"], blocks["InnerHeader"]);

    builder.SetInsertPoint(blocks["InnerHeader"]);
    auto* j_phi = builder.CreatePhi(Type::int32);
    auto* cmp_in = builder.CreateCmp(j_phi, c10);
    builder.CreateIf(cmp_in, blocks["OuterLatch"], blocks["InnerBody"]);

    builder.SetInsertPoint(blocks["InnerBody"]);
    auto* j_next = builder.CreateAdd(j_phi, c1);
    builder.CreateJump(blocks["InnerHeader"]);

    builder.SetInsertPoint(blocks["OuterLatch"]);
    auto* i_next = builder.CreateAdd(i_phi, c1);
    builder.CreateJump(blocks["OuterHeader"]);

    builder.SetInsertPoint(blocks["Exit"]);
    builder.CreateReturn(i_phi);

    i_phi->AddPhiInput(blocks["Entry"], p);
    i_phi->AddPhiInput(blocks["OuterLatch"], i_next);
    j_phi->AddPhiInput(blocks["OuterHeader"], c1);
    j_phi->AddPhiInput(blocks["InnerBody"], j_next);

    return graph;
}

// ============================================================================
// Test 3: Sequential cycles with common branching 
// ============================================================================
/* 
 *       [Entry]
 *          |
 *          v
 *      [Loop1] <---+
 *          |       |
 *          +-------+
 *          v
 *      [Branch]
 *       |    |
 *       v    v
 *   [True]  [False]
 *       |    |
 *       +----+
 *          v
 *      [Loop2] <---+
 *          |       |
 *          +-------+
 *          v
 *        [Exit]
 */
inline std::unique_ptr<Graph> BuildRegAllocGraph3(std::map<std::string_view, BasicBlock*>& blocks) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    builder.CreateNamedBlocks(blocks, "Entry", "Loop1", "Branch", "TrueBB", "FalseBB", "MergeBB", "Loop2", "Exit");

    graph->SetEntryBlock(blocks["Entry"]);

    builder.SetInsertPoint(blocks["Entry"]);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    auto* c10 = builder.CreateConstant(Type::int32, 10);
    builder.CreateJump(blocks["Loop1"]);

    builder.SetInsertPoint(blocks["Loop1"]);
    auto* l1_phi = builder.CreatePhi(Type::int32);
    auto* l1_next = builder.CreateAdd(l1_phi, c1);
    auto* l1_cmp = builder.CreateCmp(l1_next, c10);
    l1_phi->AddPhiInput(blocks["Entry"], c1);
    l1_phi->AddPhiInput(blocks["Loop1"], l1_next);
    builder.CreateIf(l1_cmp, blocks["Branch"], blocks["Loop1"]);

    builder.SetInsertPoint(blocks["Branch"]);
    builder.CreateIf(l1_cmp, blocks["TrueBB"], blocks["FalseBB"]);

    builder.SetInsertPoint(blocks["TrueBB"]);
    auto* t_val = builder.CreateAdd(l1_next, c1);
    builder.CreateJump(blocks["MergeBB"]);

    builder.SetInsertPoint(blocks["FalseBB"]);
    auto* f_val = builder.CreateAdd(l1_next, c10);
    builder.CreateJump(blocks["MergeBB"]);

    builder.SetInsertPoint(blocks["MergeBB"]);
    auto* m_phi = builder.CreatePhi(Type::int32);
    m_phi->AddPhiInput(blocks["TrueBB"], t_val);
    m_phi->AddPhiInput(blocks["FalseBB"], f_val);
    builder.CreateJump(blocks["Loop2"]);

    builder.SetInsertPoint(blocks["Loop2"]);
    auto* l2_phi = builder.CreatePhi(Type::int32);
    auto* l2_next = builder.CreateAdd(l2_phi, c1);
    auto* l2_cmp = builder.CreateCmp(l2_next, c10);
    l2_phi->AddPhiInput(blocks["MergeBB"], m_phi);
    l2_phi->AddPhiInput(blocks["Loop2"], l2_next);
    builder.CreateIf(l2_cmp, blocks["Exit"], blocks["Loop2"]);

    builder.SetInsertPoint(blocks["Exit"]);
    builder.CreateReturn(l2_next);

    return graph;
}

//helper
void RunRegAllocPipeline(TestRunner& t, Graph* graph, int num_int_regs) {
    std::cout << "\n[1] Running Dominator Analysis..." << std::endl;
    DominatorAnalysis dom(graph);
    dom.Run();

    std::cout << "[2] Running Loop Analysis..." << std::endl;
    LoopAnalyzer loops(graph, &dom);
    loops.Run();

    std::cout << "[3] Building Linear Order..." << std::endl;
    LinearOrderBuilder order_builder(graph, &loops);
    order_builder.Run();

    std::cout << "[4] Running Liveness Analysis..." << std::endl;
    LivenessAnalysis liveness(graph);
    liveness.SetLinearOrder(order_builder.GetLinearOrder());
    liveness.Run();

    std::cout << "[5] Running Linear Scan Allocator (Regs=" << num_int_regs << ")..." << std::endl;
    LinearScanAllocator allocator(graph, &liveness, num_int_regs);
    allocator.Run();

    int valid_locations = 0;
    for (const auto& bb_ptr : graph->GetBlocks()) {
        auto* bb = bb_ptr.get();
        for (auto* inst = bb->GetFirstPhi(); inst; inst = inst->GetNext()) {
            if (allocator.GetLocation(inst->GetId()).kind != Location::Kind::Unassigned) valid_locations++;
        }
        for (auto* inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
            if (allocator.GetLocation(inst->GetId()).kind != Location::Kind::Unassigned) valid_locations++;
        }
    }
    
    t.AssertNotEqual(valid_locations, 0, "No registers were allocated!");
}

//tests for graphs above
void TestRegAllocGraph1(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks;
    auto graph = BuildRegAllocGraph1(blocks);
    graph->Dump();
    
    RunRegAllocPipeline(t, graph.get(), 2); 
}

void TestRegAllocGraph2(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks;
    auto graph = BuildRegAllocGraph2(blocks);
    graph->Dump();
    
    RunRegAllocPipeline(t, graph.get(), 3);
}

void TestRegAllocGraph3(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks;
    auto graph = BuildRegAllocGraph3(blocks);
    graph->Dump();
    
    RunRegAllocPipeline(t, graph.get(), 2);
}