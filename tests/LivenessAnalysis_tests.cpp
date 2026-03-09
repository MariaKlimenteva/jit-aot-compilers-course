#include "LivenessAnalysis.hpp"
#include "LinearOrderBuilder.hpp"

void TestLivenessLinear(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* bb = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(bb);
    builder.SetInsertPoint(bb);

    // v0 = param
    // v1 = v0 + 10
    // v2 = v1 + 20
    // ret v2
    
    auto* param = builder.CreateParameter(Type::int32);
    auto* c10 = builder.CreateConstant(Type::int32, 10);
    auto* add1 = builder.CreateAdd(param, c10);
    auto* c20 = builder.CreateConstant(Type::int32, 20);
    auto* add2 = builder.CreateAdd(add1, c20);
    builder.CreateReturn(add2);

    LinearOrderBuilder order_builder(graph.get());
    order_builder.Run();
    LivenessAnalysis liveness(graph.get());
    liveness.SetLinearOrder(order_builder.GetLinearOrder());
    liveness.Run();

    auto* interval_param = liveness.GetInterval(param->GetId());
    ASSERT_NOT_EQ(interval_param, nullptr);
    ASSERT_EQ(interval_param->ranges.size(), size_t(1));
    ASSERT_EQ(interval_param->ranges[0].begin, 0);
    ASSERT_EQ(interval_param->ranges[0].end, 4);

    auto* interval_add1 = liveness.GetInterval(add1->GetId());
    ASSERT_NOT_EQ(interval_add1, nullptr);
    ASSERT_EQ(interval_add1->ranges.size(), size_t(1));
    ASSERT_EQ(interval_add1->ranges[0].begin, 4);
    ASSERT_EQ(interval_add1->ranges[0].end, 8);
    
    auto* interval_add2 = liveness.GetInterval(add2->GetId());
    ASSERT_NOT_EQ(interval_add2, nullptr);
    ASSERT_EQ(interval_add2->ranges.size(), size_t(1));
    ASSERT_EQ(interval_add2->ranges[0].begin, 8);
    ASSERT_EQ(interval_add2->ranges[0].end, 10);
}

void TestLivenessIf(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());

    auto* entry = graph->CreateNewBasicBlock();
    auto* then_bb = graph->CreateNewBasicBlock();
    auto* else_bb = graph->CreateNewBasicBlock();
    auto* merge_bb = graph->CreateNewBasicBlock();
    graph->SetEntryBlock(entry);
    builder.SetInsertPoint(entry);
    auto* param = builder.CreateParameter(Type::int32);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    builder.CreateIf(param, then_bb, else_bb);
    builder.SetInsertPoint(then_bb);
    auto* add_then = builder.CreateAdd(param, c1);
    builder.CreateJump(merge_bb);

    builder.SetInsertPoint(else_bb);
    auto* mul_else = builder.CreateMul(param, c1);
    builder.CreateJump(merge_bb);

    builder.SetInsertPoint(merge_bb);
    auto* phi = builder.CreatePhi(Type::int32);
    phi->AddPhiInput(then_bb, add_then);
    phi->AddPhiInput(else_bb, mul_else);
    builder.CreateReturn(phi);

    LinearOrderBuilder order_builder(graph.get(), nullptr);
    order_builder.Run();
    LivenessAnalysis liveness(graph.get());
    liveness.SetLinearOrder(order_builder.GetLinearOrder());
    liveness.Run();
    auto* interval_param = liveness.GetInterval(param->GetId());
    ASSERT_NOT_EQ(interval_param, nullptr);
    ASSERT_EQ(interval_param->ranges.size(), size_t(1));
    ASSERT_EQ(interval_param->ranges[0].begin, 0);
    ASSERT_EQ(interval_param->ranges[0].end, 6);
}

void TestLivenessLoop(TestRunner& t) {
    auto graph = BuildFactorialGraph();
    
    DominatorAnalysis dom(graph.get());
    dom.Run();

    LoopAnalyzer loops(graph.get(), &dom);
    loops.Run();

    LinearOrderBuilder order_builder(graph.get(), &loops);
    order_builder.Run();
    
    LivenessAnalysis liveness(graph.get());
    liveness.SetLinearOrder(order_builder.GetLinearOrder());
    liveness.Run();

    Instruction *result_phi = nullptr, *i_phi = nullptr;
    Instruction *new_result_mul = nullptr, *new_i_add = nullptr;

    const auto& blocks = graph->GetBlocks();
    auto it = blocks.begin();
    (*it++).get();
    BasicBlock* header = (*it++).get();
    BasicBlock* body = (*it++).get();

    for(auto* inst = header->GetFirstPhi(); inst; inst=inst->GetNext()) {
        if (!result_phi) result_phi = inst;
        else i_phi = inst;
    }
    
    for(auto* inst = body->GetFirstInst(); inst; inst=inst->GetNext()) {
        if (inst->GetOpcode() == Opcode::Mul) new_result_mul = inst;
        if (inst->GetOpcode() == Opcode::Add) new_i_add = inst;
    }

    ASSERT_NOT_EQ(result_phi, nullptr);
    ASSERT_NOT_EQ(new_result_mul, nullptr);
    
    auto* interval_res = liveness.GetInterval(result_phi->GetId());
    ASSERT_NOT_EQ(interval_res, nullptr);
    
    int phi_pos = result_phi->GetLifePosition();
    int mul_pos = new_result_mul->GetLifePosition();
    
    bool found_correct_range = false;
    for(const auto& range : interval_res->ranges) {
        if (range.begin <= phi_pos && range.end >= mul_pos) {
            found_correct_range = true;
            break;
        }
    }
    ASSERT_EQ(found_correct_range, true);
}