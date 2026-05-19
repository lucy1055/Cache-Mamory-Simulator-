#include "cache.hpp"

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

static std::vector<std::uint64_t> loadTrace(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Unable to open trace file: " + path);
    }

    std::vector<std::uint64_t> addresses;
    std::string token;
    while (in >> token) {
        std::uint64_t addr = 0;
        if (token.rfind("0x", 0) == 0 || token.rfind("0X", 0) == 0) {
            addr = std::stoull(token, nullptr, 16);
        } else {
            addr = std::stoull(token, nullptr, 10);
        }
        addresses.push_back(addr);
    }
    return addresses;
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: " << argv[0] << " <trace-file>\n";
            return 1;
        }

        const auto trace = loadTrace(argv[1]);

        SetAssociativeCache l1(32 * 1024, 64, 8, ReplacementPolicy::LRU, 10, 1);
        SetAssociativeCache l2(256 * 1024, 64, 8, ReplacementPolicy::FIFO, 120, 8);

        const auto result = simulateTwoLevel(trace, l1, l2);

        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Trace length: " << trace.size() << "\n";
        std::cout << "L1 hit rate: " << result.l1.hitRate() * 100.0 << "%\n";
        std::cout << "L1 miss rate: " << result.l1.missRate() * 100.0 << "%\n";
        std::cout << "L2 hit rate: " << result.l2.hitRate() * 100.0 << "%\n";
        std::cout << "L2 miss rate: " << result.l2.missRate() * 100.0 << "%\n";
        std::cout << "Average latency (cycles): " << result.averageLatencyCycles << "\n";

        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 1;
    }
}
