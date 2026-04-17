#pragma once

#include "Graph.hpp"
#include "DominatorAnalysis.hpp"
#include "LoopAnalyzer.hpp"
#include <vector>
#include <algorithm>

class CheckElimination {
public:
    CheckElimination(Graph* graph, DominatorAnalysis* dom, LoopAnalyzer* loops)
        : graph_(graph), dom_(dom), loops_(loops) {}

    void Run();
    int GetRemovedCount() const { return removed_count_; }
    int GetHoistedCount() const { return hoisted_count_; }
private:
    Graph* graph_;
    DominatorAnalysis* dom_;
    LoopAnalyzer* loops_;
    int removed_count_ = 0;
    int hoisted_count_ = 0;

    bool IsSameNullCheck(Instruction* a, Instruction* b) const;
    bool IsSameBoundsCheck(Instruction* a, Instruction* b) const;
    bool IsDominatedBySameCheck(Instruction* check);
    bool IsLoopInvariant(Instruction* inst, Loop* loop) const;
    BasicBlock* GetOrCreatePreheader(Loop* loop);
    void ReplaceAllUses(Instruction* oldInst, Instruction* newInst);
    void HoistChecksFromLoops();
    void RemoveDominatedChecks();

    static void RedirectEdge(BasicBlock* from, BasicBlock* to_new, BasicBlock* to_old) {
        to_old->RemovePred(from);
        from->RemoveSucc(to_old);
        from->AddSucc(to_new);
        to_new->AddPred(from);
    }

    static void FixPhiBlocks(BasicBlock* old_pred, BasicBlock* new_pred, BasicBlock* header) {
        for (auto* phi_inst = header->GetFirstPhi(); phi_inst; phi_inst = phi_inst->GetNext()) {
            auto* phi = static_cast<PhiInst*>(phi_inst);
            phi->ReplaceBlock(old_pred, new_pred);
        }
    }
};
