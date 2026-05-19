#pragma once

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <vector>

enum class ReplacementPolicy {
    LRU,
    FIFO
};

struct CacheStats {
    std::uint64_t accesses = 0;
    std::uint64_t hits = 0;
    std::uint64_t misses = 0;

    double hitRate() const;
    double missRate() const;
};

class SetAssociativeCache {
public:
    SetAssociativeCache(std::size_t cacheSizeBytes,
                        std::size_t blockSizeBytes,
                        std::size_t associativity,
                        ReplacementPolicy policy,
                        unsigned memoryPenaltyCycles,
                        unsigned hitCycles);

    bool access(std::uint64_t address);
    const CacheStats& stats() const;
    double averageAccessLatency() const;
    unsigned hitCycles() const;
    unsigned missCycles() const;

private:
    struct Line {
        std::uint64_t tag = 0;
        bool valid = false;
    };

    struct Set {
        std::vector<Line> lines;
        std::deque<std::size_t> order;
        std::unordered_map<std::uint64_t, std::size_t> tagToWay;
    };

    std::size_t getSetIndex(std::uint64_t address) const;
    std::uint64_t getTag(std::uint64_t address) const;

    std::vector<Set> sets_;
    std::size_t blockSize_;
    std::size_t associativity_;
    ReplacementPolicy policy_;
    unsigned memoryPenaltyCycles_;
    unsigned hitCycles_;
    CacheStats stats_;
};

struct TwoLevelResult {
    CacheStats l1;
    CacheStats l2;
    double averageLatencyCycles = 0.0;
};

TwoLevelResult simulateTwoLevel(const std::vector<std::uint64_t>& addresses,
                                SetAssociativeCache& l1,
                                SetAssociativeCache& l2);
