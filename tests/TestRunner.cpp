#include "IRBuilder.hpp"
#include <iostream>
#include <cassert>
#include "BuildGraphs.hpp"
#include "DominatorAnalysis.hpp"
#include "TestRunner.hpp"

#define ASSERT_EQ(left, right) \
    t.AssertEqual(left, right, #left " == " #right " at " __FILE__ ":" + std::to_string(__LINE__))

void TestFactorialGraph() {
    auto graph = BuildFactorialGraph();
    assert(graph != nullptr);
    graph->Dump();
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
    first_inst->Dump();
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
}
int main() {
    TestRunner runner;
    
    runner.AddTest("Example 1 Dominators", TestExample1);
    runner.AddTest("Example 2 Dominators", TestExample2);
    runner.AddTest("Example 3 Dominators", TestExample3);
    
    runner.RunAllTests();
    return (runner.GetFailedCount() > 0) ? 1 : 0;
}