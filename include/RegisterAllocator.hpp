#pragma once
#include "LivenessAnalysis.hpp"
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <algorithm>

struct Location {
    enum class Kind { Register, Stack, Unassigned } kind = Kind::Unassigned;
    int id = -1;
    
    std::string ToString() const {
        if (kind == Kind::Register) return "R" + std::to_string(id);
        if (kind == Kind::Stack) return "Stack[" + std::to_string(id) + "]";
        return "Unassigned";
    }
};

class LinearScanAllocator {
public:
    LinearScanAllocator(Graph* graph, LivenessAnalysis* liveness, int num_int_regs, int num_float_regs = 0)
        : graph_(graph), liveness_(liveness), R_int(num_int_regs), R_float(num_float_regs) {}

    void Run() {
        InitializeFreeRegisters();
        
        std::vector<const LiveInterval*> sorted_intervals;
        for (const auto& bb_ptr : graph_->GetBlocks()) {
            auto* bb = bb_ptr.get();
            for (auto* inst = bb->GetFirstPhi(); inst; inst = inst->GetNext()) {
                if (auto* iv = liveness_->GetInterval(inst->GetId())) sorted_intervals.push_back(iv);
            }
            for (auto* inst = bb->GetFirstInst(); inst; inst = inst->GetNext()) {
                if (auto* iv = liveness_->GetInterval(inst->GetId())) sorted_intervals.push_back(iv);
            }
        }

        std::sort(sorted_intervals.begin(), sorted_intervals.end(),[](const LiveInterval* a, const LiveInterval* b) {
            return GetStart(a) < GetStart(b);
        });

        for (const auto* i : sorted_intervals) {
            ExpireOldIntervals(i);
            
            if (active_.size() == static_cast<size_t>(R_int)) {
                SpillAtInterval(i);
            } else {
                int reg = free_registers_.front();
                free_registers_.erase(free_registers_.begin());
                allocations_[i->reg_id] = {Location::Kind::Register, reg};
                
                InsertActive(i);
            }
        }
        
        DumpAllocations();
    }

    Location GetLocation(int reg_id) { return allocations_[reg_id]; }

private:
    Graph* graph_;
    LivenessAnalysis* liveness_;
    int R_int;
    int R_float;
    
    std::vector<int> free_registers_;
    std::vector<const LiveInterval*> active_;
    std::map<int, Location> allocations_;
    int next_stack_slot_ = 0;

    static int GetStart(const LiveInterval* iv) { return iv->ranges.empty() ? 0 : iv->ranges.front().begin; }
    static int GetEnd(const LiveInterval* iv) { return iv->ranges.empty() ? 0 : iv->ranges.back().end; }

    void InitializeFreeRegisters() {
        for (int i = 0; i < R_int; ++i) free_registers_.push_back(i);
    }

    void InsertActive(const LiveInterval* iv) {
        active_.push_back(iv);
        std::sort(active_.begin(), active_.end(),[](const LiveInterval* a, const LiveInterval* b) {
            return GetEnd(a) < GetEnd(b);
        });
    }

    void ExpireOldIntervals(const LiveInterval* i) {
        auto it = active_.begin();
        while (it != active_.end()) {
            const LiveInterval* j = *it;
            if (GetEnd(j) >= GetStart(i)) {
                return; 
            }
            Location loc = allocations_[j->reg_id];
            if (loc.kind == Location::Kind::Register) {
                free_registers_.push_back(loc.id);
                std::sort(free_registers_.begin(), free_registers_.end()); 
            }
            it = active_.erase(it);
        }
    }

    void SpillAtInterval(const LiveInterval* i) {
        const LiveInterval* spill = active_.back();
        
        if (GetEnd(spill) > GetEnd(i)) {
            allocations_[i->reg_id] = allocations_[spill->reg_id];
            allocations_[spill->reg_id] = {Location::Kind::Stack, next_stack_slot_++};
            
            active_.pop_back();
            InsertActive(i);
        } else {
            allocations_[i->reg_id] = {Location::Kind::Stack, next_stack_slot_++};
        }
    }

    void DumpAllocations() const {
        std::cout << "\n=== Register Allocation ===\n";
        for (const auto& [id, loc] : allocations_) {
            std::cout << "v" << id << " -> " << loc.ToString() << "\n";
        }
        std::cout << "===========================\n";
    }
};