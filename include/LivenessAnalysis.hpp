#pragma once
#include "Graph.hpp"
#include <vector>
#include <map>
#include <set>

struct LiveRange {
    int begin;
    int end;
    bool operator==(const LiveRange& other) const {
        return begin == other.begin && end == other.end;
    }
};

struct LiveInterval {
    int reg_id;
    std::vector<LiveRange> ranges;
    void AddRange(int from, int to) {
        if (from >= to) return;
        if (ranges.empty()) {
            ranges.push_back({from, to});
            return;
        }

        auto& last = ranges.back();
        if (from <= last.end && to >= last.begin) {
            last.begin = std::min(last.begin, from);
            last.end = std::max(last.end, to);
        } else {
            ranges.push_back({from, to});
        }
    }
    void SetFrom(int from) {
        if (!ranges.empty()) {
            ranges.back().begin = from;
        } else {
            ranges.push_back({from, from + 2});
        }
    }
};

class LivenessAnalysis {
public:
    explicit LivenessAnalysis(Graph* graph) : graph_(graph) {}

    void Run();
    const std::vector<BasicBlock*>& GetLinearOrder() const { return linear_blocks_; }
    const LiveInterval* GetInterval(int inst_id) const { 
        auto it = intervals_.find(inst_id);
        if (it != intervals_.end()) {
            return &it->second;
        }
        return nullptr;
    }
    void Dump() {
        std::cout << "Liveness Intervals:\n";
        for (auto& [id, interval] : intervals_) {
            std::cout << "v" << id << ": ";
            for (auto& r : interval.ranges) {
                std::cout << "[" << r.begin << ", " << r.end << ") ";
            }
            std::cout << "\n";
        }
    }
private:
    Graph* graph_;
    std::vector<BasicBlock*> linear_blocks_;
    std::map<int, LiveInterval> intervals_;
    std::map<BasicBlock*, std::set<int>> live_in_;
    std::map<BasicBlock*, std::set<int>> live_out_;
    
    void ComputeLinearOrder();
    void ComputeRPO(BasicBlock* bb, std::set<BasicBlock*>& visited);
    void NumberInstructions();
    void ComputeGlobalLiveness();
    void BuildIntervals();
};