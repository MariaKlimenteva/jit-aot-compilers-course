#include "IRBuilder.hpp"
#include <cstdint>
#include <iostream>
#include <cassert>
#include "BuildGraphs.hpp"
#include "DominatorAnalysis.hpp"
#include "TestRunner.hpp"
#include "LoopAnalyzer.hpp"
#include "Optimizer.hpp"

std::ostream& operator<<(std::ostream& os, const Opcode& op) {
    return os << static_cast<int>(op);
}

#define ASSERT_EQ(left, right) \
    t.AssertEqual(left, right, #left " == " #right " at " __FILE__ ":" + std::to_string(__LINE__))
#define ASSERT_NOT_EQ(left, right) \
    t.AssertNotEqual(left, right, #left " != " #right " at " __FILE__ ":" + std::to_string(__LINE__))
void TestFactorialGraph(TestRunner& t) {
    auto graph = BuildFactorialGraph();
    assert(graph != nullptr);
    graph->Dump();
    assert(graph->GetBlocks().size() == 4);

    const auto& blocks = graph->GetBlocks();
    auto it = blocks.begin();
    
    BasicBlock* entry_bb = (*it++).get();
    BasicBlock* loop_header_bb = (*it++).get();
    BasicBlock* loop_body_bb = (*it++).get();
    BasicBlock* exit_bb = (*it).get();

    ASSERT_EQ(entry_bb->GetPreds().empty(), true);
    assert(entry_bb->GetSuccs().size() != 1);
    ASSERT_EQ(entry_bb->GetSuccs()[0],loop_header_bb);
    assert(entry_bb->GetLastInst()->GetOpcode() == Opcode::Jump);

    assert(loop_header_bb->GetPreds().size() != 2); 
    assert(loop_header_bb->GetSuccs().size() != 2);
    assert(loop_header_bb->GetLastInst()->GetOpcode() == Opcode::If);
    
    auto* first_inst = loop_header_bb->GetFirstInst();
    first_inst->Dump();
    assert(first_inst->GetOpcode() == Opcode::Phi);
    auto* phi_result = static_cast<PhiInst*>(first_inst);
    assert(phi_result->GetInputs().size() != 2); 
    
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
    ASSERT_EQ(from_entry_found && from_loop_body_found, true);

    assert(loop_body_bb->GetPreds().size() != 1); 
    ASSERT_EQ(loop_body_bb->GetPreds()[0], loop_header_bb);
    assert(loop_body_bb->GetSuccs().size() != 1); 
    ASSERT_EQ(loop_body_bb->GetSuccs()[0], loop_header_bb);
    Instruction* current = loop_body_bb->GetFirstInst();
    assert(current->GetOpcode() == Opcode::Mul);
    current = current->GetNext();
    assert(current->GetOpcode() == Opcode::Add);
    current = current->GetNext();
    assert(current->GetOpcode() == Opcode::Jump);

    assert(exit_bb->GetPreds().size() != 1);
    ASSERT_EQ(exit_bb->GetPreds()[0], loop_header_bb);
    ASSERT_EQ(exit_bb->GetSuccs().empty(), true);
    assert(exit_bb->GetLastInst()->GetOpcode() == Opcode::Ret);

    ASSERT_EQ(exit_bb->GetLastInst()->GetInputs()[0], phi_result);
    
    std::cout << "Factorial graph test passed successfully!" << std::endl;
}

void TestExample1(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks {};
    auto graph = BuildExample1Graph(blocks);
    graph->Dump();
    DominatorAnalysis analysis(graph.get());
    analysis.Run();
    ASSERT_EQ(analysis.GetIdom(blocks["A"]), nullptr);
    ASSERT_EQ(analysis.GetIdom(blocks["B"]), blocks["A"]);
    ASSERT_EQ(analysis.GetIdom(blocks["F"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["E"]), blocks["F"]);
    ASSERT_EQ(analysis.GetIdom(blocks["G"]), blocks["F"]);
    std::cout << "idom(D) == " << analysis.GetIdom(blocks["D"])->GetId() << std::endl;
    ASSERT_EQ(analysis.GetIdom(blocks["D"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["C"]), blocks["B"]);

    LoopAnalyzer loop_analyzer(graph.get(), &analysis);
    loop_analyzer.Run();
    loop_analyzer.Dump();
    ASSERT_EQ(loop_analyzer.GetLoops().size(), static_cast<size_t>(0));
}

void TestExample2(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks {};
    auto graph = BuildExample2Graph(blocks);
    graph->Dump();
    DominatorAnalysis analysis(graph.get());
    analysis.Run();
    ASSERT_EQ(analysis.GetIdom(blocks["A"]), nullptr);
    ASSERT_EQ(analysis.GetIdom(blocks["B"]), blocks["A"]);
    ASSERT_EQ(analysis.GetIdom(blocks["C"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["D"]), blocks["C"]);
    ASSERT_EQ(analysis.GetIdom(blocks["E"]), blocks["D"]);
    ASSERT_EQ(analysis.GetIdom(blocks["F"]), blocks["E"]);
    ASSERT_EQ(analysis.GetIdom(blocks["G"]), blocks["F"]);
    ASSERT_EQ(analysis.GetIdom(blocks["H"]), blocks["G"]);
    ASSERT_EQ(analysis.GetIdom(blocks["I"]), blocks["G"]);
    ASSERT_EQ(analysis.GetIdom(blocks["J"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["K"]), blocks["I"]);
    LoopAnalyzer loop_analyzer(graph.get(), &analysis);
    loop_analyzer.Run();
    loop_analyzer.Dump();
    const auto& loops = loop_analyzer.GetLoops();
    ASSERT_NOT_EQ(loops.size(), static_cast<size_t>(0));
    bool found_b_loop = false;
    for(const auto& l : loops) {
        if (l->header == blocks["B"]) {
            found_b_loop = true;
            ASSERT_EQ(l->Contains(blocks["H"]), true);
            ASSERT_EQ(l->Contains(blocks["C"]), true);
        }
    }
    ASSERT_EQ(found_b_loop, true);
}

void TestExample3(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks {};
    auto graph = BuildExample3Graph(blocks);
    graph->Dump();
    DominatorAnalysis analysis(graph.get());
    analysis.Run();
    ASSERT_EQ(analysis.GetIdom(blocks["A"]), nullptr);
    ASSERT_EQ(analysis.GetIdom(blocks["B"]), blocks["A"]);
    ASSERT_EQ(analysis.GetIdom(blocks["C"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["D"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["E"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["F"]), blocks["E"]);
    ASSERT_EQ(analysis.GetIdom(blocks["G"]), blocks["B"]);
    ASSERT_EQ(analysis.GetIdom(blocks["H"]), blocks["F"]);
    ASSERT_EQ(analysis.GetIdom(blocks["I"]), blocks["B"]);
    LoopAnalyzer loop_analyzer(graph.get(), &analysis);
    loop_analyzer.Run();
    loop_analyzer.Dump();
    const auto& loops = loop_analyzer.GetLoops();
    bool found_b_header = false;
    for(const auto& l : loops) {
        if(l->header == blocks["B"]) found_b_header = true;
    }
    ASSERT_EQ(found_b_header, true);
}

void TestExample4(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks;
    auto graph = BuildExample4Graph(blocks);
    
    DominatorAnalysis dom(graph.get());
    dom.Run();
    
    LoopAnalyzer loops(graph.get(), &dom);
    loops.Run();
    loops.Dump();
    
    // A->B, B->{C,D}, D->E, E->B
    // Цикл: B->D->E->B.
    // Header: B. Latch: E.
    // C - выход из цикла
    
    ASSERT_EQ(loops.GetLoops().size(), static_cast<size_t>(1));
    Loop* l = loops.GetLoops()[0].get();
    
    ASSERT_EQ(l->header, blocks["B"]);
    ASSERT_EQ(l->Contains(blocks["B"]), true);
    ASSERT_EQ(l->Contains(blocks["D"]), true);
    ASSERT_EQ(l->Contains(blocks["E"]), true);
    
    ASSERT_EQ(l->Contains(blocks["C"]), false);
    ASSERT_EQ(l->Contains(blocks["A"]), false);
}

void TestExample5(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks;
    auto graph = BuildExample5Graph(blocks);
    
    DominatorAnalysis dom(graph.get());
    dom.Run();
    
    LoopAnalyzer loops(graph.get(), &dom);
    loops.Run();
    loops.Dump();
    
    // A->B->C->D->E->B. C->F, D->F.
    // Цикл: B-C-D-E.
    // Header: B.
    // F и A не в цикле
    
    ASSERT_EQ(loops.GetLoops().size(), static_cast<size_t>(1));
    Loop* l = loops.GetLoops()[0].get();
    
    ASSERT_EQ(l->header, blocks["B"]);
    ASSERT_EQ(l->Contains(blocks["B"]), true);
    ASSERT_EQ(l->Contains(blocks["C"]), true);
    ASSERT_EQ(l->Contains(blocks["D"]), true);
    ASSERT_EQ(l->Contains(blocks["E"]), true);
    
    ASSERT_EQ(l->Contains(blocks["F"]), false);
    ASSERT_EQ(l->Contains(blocks["A"]), false);
}

void TestExample6(TestRunner& t) {
    std::map<std::string_view, BasicBlock*> blocks;
    auto graph = BuildExample6Graph(blocks);
    
    DominatorAnalysis dom(graph.get());
    dom.Run();
    
    LoopAnalyzer loops(graph.get(), &dom);
    loops.Run();
    loops.Dump();
    
    // Outer: A->...->H->A (Header A)
    // Inner: B->...->G->B (Header B)
    // A->B, B->{C,D}, C->{E,F}, F->G, G->{H,B}, H->A
    
    ASSERT_EQ(dom.GetIdom(blocks["B"]), blocks["A"]);
    ASSERT_EQ(dom.GetIdom(blocks["H"]), blocks["G"]);
    
    const auto& all_loops = loops.GetLoops();
    ASSERT_EQ(all_loops.size(), static_cast<size_t>(2));
    
    Loop* loopA = nullptr;
    Loop* loopB = nullptr;
    
    for(const auto& l : all_loops) {
        if (l->header == blocks["A"]) loopA = l.get();
        if (l->header == blocks["B"]) loopB = l.get();
    }
    
    ASSERT_NOT_EQ(loopA, nullptr);
    ASSERT_NOT_EQ(loopB, nullptr);

    ASSERT_EQ(loopB->parent_loop, loopA);
    
    ASSERT_EQ(loopB->Contains(blocks["B"]), true);
    ASSERT_EQ(loopB->Contains(blocks["G"]), true);
    ASSERT_EQ(loopB->Contains(blocks["F"]), true); 
    ASSERT_EQ(loopB->Contains(blocks["H"]), false);
    
    ASSERT_EQ(loopA->Contains(blocks["A"]), true);
    ASSERT_EQ(loopA->Contains(blocks["H"]), true);
    ASSERT_EQ(loopA->Contains(blocks["B"]), true);
}

void TestLoops(TestRunner& t) {
    auto graph = BuildFactorialGraph(); 
    DominatorAnalysis dom(graph.get());
    dom.Run();

    LoopAnalyzer loop_analyzer(graph.get(), &dom);
    loop_analyzer.Run();

    const auto& loops = loop_analyzer.GetLoops();
    ASSERT_EQ(loops.size(), static_cast<size_t>(1)); 

    Loop* loop = loops[0].get();
    
    const auto& blocks = graph->GetBlocks();
    auto it = blocks.begin();
    BasicBlock* entry = (*it++).get();
    BasicBlock* header = (*it++).get();
    BasicBlock* body = (*it++).get();
    BasicBlock* exit = (*it).get();

    ASSERT_EQ(loop->header, header);
    
    ASSERT_EQ(loop->Contains(header), true);
    ASSERT_EQ(loop->Contains(body), true);
    ASSERT_EQ(loop->Contains(entry), false);
    ASSERT_EQ(loop->Contains(exit), false);

    Loop* root = loop_analyzer.GetRootLoop();
    ASSERT_EQ(root->sub_loops.size(), static_cast<size_t>(1));
    ASSERT_EQ(root->sub_loops[0], loop);
    ASSERT_EQ(loop->parent_loop, root);
    loop_analyzer.Dump();
    std::cout << "Loop Analysis test passed!" << std::endl;
}

int64_t GetConstVal(Instruction* inst) {
    if (!inst) {
        throw std::runtime_error("GetConstVal failed: Instruction is nullptr");
    }
    
    auto* const_inst = dynamic_cast<ConstantInst*>(inst);
    if (!const_inst) {
        throw std::runtime_error("GetConstVal failed: Instruction is NOT a ConstantInst (Opcode: " + 
                                 std::to_string(static_cast<int>(inst->GetOpcode())) + ")");
    }
    
    return std::visit([](auto arg) { return static_cast<int64_t>(arg); }, const_inst->GetValue());
}

void TestConstantFolding(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* bb = graph->CreateNewBasicBlock();
    builder.SetInsertPoint(bb);

    auto* c10 = builder.CreateConstant(Type::int32, 10);
    auto* c2 = builder.CreateConstant(Type::int32, 2);
    auto* mul = builder.CreateMul(c10, c2);
    
    auto* ret = builder.CreateReturn(mul);

    Optimizer opt(graph.get());
    opt.Run();

    auto* res = ret->GetInputs()[0];
    ASSERT_EQ(res->GetOpcode(), Opcode::Const);
    ASSERT_EQ(GetConstVal(res), 20);
    
    ASSERT_NOT_EQ(res, mul);
}

void TestPeepholeMul(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* bb = graph->CreateNewBasicBlock();
    builder.SetInsertPoint(bb);

    auto* param = builder.CreateParameter(Type::int32);
    auto* c1 = builder.CreateConstant(Type::int32, 1);
    auto* c0 = builder.CreateConstant(Type::int32, 0);

    auto* mul_one = builder.CreateMul(param, c1);
    auto* ret1 = builder.CreateReturn(mul_one);

    auto* mul_zero = builder.CreateMul(param, c0);
    auto* ret2 = builder.CreateReturn(mul_zero); 

    Optimizer opt(graph.get());
    opt.Run();

    ASSERT_EQ(ret1->GetInputs()[0], param);

    auto* res_zero = ret2->GetInputs()[0];
    ASSERT_EQ(res_zero->GetOpcode(), Opcode::Const);
    ASSERT_EQ(GetConstVal(res_zero), 0);
}

void TestPeepholeOr(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* bb = graph->CreateNewBasicBlock();
    builder.SetInsertPoint(bb);

    auto* param = builder.CreateParameter(Type::int32);
    auto* c0 = builder.CreateConstant(Type::int32, 0);
    auto* cm1 = builder.CreateConstant(Type::int32, -1);

    auto* or_zero = builder.CreateOr(param, c0);
    auto* ret1 = builder.CreateReturn(or_zero);

    auto* or_minus1 = builder.CreateOr(param, cm1);
    auto* ret2 = builder.CreateReturn(or_minus1);

    auto* or_self = builder.CreateOr(param, param);
    auto* ret3 = builder.CreateReturn(or_self);

    Optimizer opt(graph.get());
    opt.Run();

    ASSERT_EQ(ret1->GetInputs()[0], param);

    auto* res_m1 = ret2->GetInputs()[0];
    ASSERT_EQ(res_m1->GetOpcode(), Opcode::Const);
    ASSERT_EQ(GetConstVal(res_m1), -1);

    ASSERT_EQ(ret3->GetInputs()[0], param);
}

void TestPeepholeAshr(TestRunner& t) {
    auto graph = std::make_unique<Graph>();
    IRBuilder builder(graph.get());
    auto* bb = graph->CreateNewBasicBlock();
    builder.SetInsertPoint(bb);

    auto* param = builder.CreateParameter(Type::int32);
    auto* c0 = builder.CreateConstant(Type::int32, 0);

    auto* shr_zero = builder.CreateShr(param, c0);
    auto* ret1 = builder.CreateReturn(shr_zero);

    auto* zero_shr = builder.CreateShr(c0, param);
    auto* ret2 = builder.CreateReturn(zero_shr);

    Optimizer opt(graph.get());
    opt.Run();

    ASSERT_EQ(ret1->GetInputs()[0], param);
    
    auto* res_zero = ret2->GetInputs()[0];
    ASSERT_EQ(res_zero->GetOpcode(), Opcode::Const);
    ASSERT_EQ(GetConstVal(res_zero), 0);
}

int main() {
    TestRunner runner;
    
    runner.AddTest("Example 1 Dominators & Loops", TestExample1);
    runner.AddTest("Example 2 Dominators & Loops", TestExample2);
    runner.AddTest("Example 3 Dominators & Loops", TestExample3);
    runner.AddTest("Loop Analysis Factorial", TestLoops);
    runner.AddTest("Opt: Constant Folding", TestConstantFolding);
    runner.AddTest("Opt: Peephole MUL", TestPeepholeMul);
    runner.AddTest("Opt: Peephole OR", TestPeepholeOr);
    runner.AddTest("Opt: Peephole ASHR", TestPeepholeAshr);
    runner.AddTest("Loop: Example 4 (Basic Loop)", TestExample4);
    runner.AddTest("Loop: Example 5 (Shared Exit)", TestExample5);
    runner.AddTest("Loop: Example 6 (Nested Loops)", TestExample6);
    runner.RunAllTests();
    return (runner.GetFailedCount() > 0) ? 1 : 0;
}