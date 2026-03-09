#include "LoopAnalyzer.hpp"
#include "TestRunner.hpp"
#include "IRBuilder.hpp"
#include "BuildGraphs.hpp"
#include "TestsUtils.hpp"

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