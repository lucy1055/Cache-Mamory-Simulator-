#include "cache.hpp"

#include <algorithm>
#include <stdexcept>

double CacheStats::hitRate() const {
    return accesses == 0 ? 0.0 : static_cast<double>(hits) / static_cast<double>(accesses);
}

double CacheStats::missRate() const {
    return accesses == 0 ? 0.0 : static_cast<double>(misses) / static_cast<double>(accesses);
}

SetAssociativeCache::SetAssociativeCache(std::size_t cacheSizeBytes,
                                         std::size_t blockSizeBytes,
                                         std::size_t associativity,
                                         ReplacementPolicy policy,
                                         unsigned memoryPenaltyCycles,
                                         unsigned hitCycles)
    : blockSize_(blockSizeBytes),
      associativity_(associativity),
      policy_(policy),
      memoryPenaltyCycles_(memoryPenaltyCycles),
      hitCycles_(hitCycles) {
    if (blockSizeBytes == 0 || associativity == 0 || cacheSizeBytes == 0) {
        throw std::invalid_argument("Cache size, block size, and associativity must be non-zero");
    }

    const std::size_t lineCount = cacheSizeBytes / blockSizeBytes;
    const std::size_t setCount = lineCount / associativity;
    if (setCount == 0 || lineCount % associativity != 0) {
        throw std::invalid_argument("Cache geometry invalid for given size/block/associativity");
    }

    sets_.resize(setCount);
    for (auto& set : sets_) {
        set.lines.resize(associativity_);
        for (std::size_t way = 0; way < associativity_; ++way) {
            set.order.push_back(way);
        }
    }
}

bool SetAssociativeCache::access(std::uint64_t address) {
    ++stats_.accesses;
    const std::size_t setIndex = getSetIndex(address);
    const std::uint64_t tag = getTag(address);
    Set& set = sets_[setIndex];

    auto hitIt = set.tagToWay.find(tag);
    if (hitIt != set.tagToWay.end()) {
        ++stats_.hits;
        if (policy_ == ReplacementPolicy::LRU) {
            const std::size_t hitWay = hitIt->second;
            auto orderIt = std::find(set.order.begin(), set.order.end(), hitWay);
            if (orderIt != set.order.end()) {
                set.order.erase(orderIt);
                set.order.push_back(hitWay);
            }
        }
        return true;
    }

    ++stats_.misses;
    const std::size_t victimWay = set.order.front();
    set.order.pop_front();

    Line& victimLine = set.lines[victimWay];
    if (victimLine.valid) {
        set.tagToWay.erase(victimLine.tag);
    }

    victimLine.valid = true;
    victimLine.tag = tag;
    set.tagToWay[tag] = victimWay;
    set.order.push_back(victimWay);

    return false;
}

const CacheStats& SetAssociativeCache::stats() const {
    return stats_;
}

unsigned SetAssociativeCache::hitCycles() const {
    return hitCycles_;
}

unsigned SetAssociativeCache::missCycles() const {
    return hitCycles_ + memoryPenaltyCycles_;
}

double SetAssociativeCache::averageAccessLatency() const {
    if (stats_.accesses == 0) {
        return 0.0;
    }

    const double totalCycles = static_cast<double>(stats_.hits) * hitCycles_ +
                               static_cast<double>(stats_.misses) * (hitCycles_ + memoryPenaltyCycles_);
    return totalCycles / static_cast<double>(stats_.accesses);
}

std::size_t SetAssociativeCache::getSetIndex(std::uint64_t address) const {
    const std::uint64_t blockNumber = address / blockSize_;
    return static_cast<std::size_t>(blockNumber % sets_.size());
}

std::uint64_t SetAssociativeCache::getTag(std::uint64_t address) const {
    const std::uint64_t blockNumber = address / blockSize_;
    return blockNumber / sets_.size();
}

TwoLevelResult simulateTwoLevel(const std::vector<std::uint64_t>& addresses,
                                SetAssociativeCache& l1,
                                SetAssociativeCache& l2) {
    double totalCycles = 0.0;

    for (const std::uint64_t address : addresses) {
        if (l1.access(address)) {
            totalCycles += l1.hitCycles();
            continue;
        }

        if (l2.access(address)) {
            totalCycles += l1.hitCycles() + l2.hitCycles();
        } else {
            totalCycles += l1.hitCycles() + l2.missCycles();
        }
    }

    TwoLevelResult result;
    result.l1 = l1.stats();
    result.l2 = l2.stats();
    result.averageLatencyCycles = addresses.empty() ? 0.0 : totalCycles / static_cast<double>(addresses.size());
    return result;
}
