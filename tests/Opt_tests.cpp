#include "DominatorAnalysis.hpp"
#include "Optimizer.hpp"

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
