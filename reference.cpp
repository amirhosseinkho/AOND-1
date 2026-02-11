#include <vector>
#include <cstdint>
#include <algorithm>

struct PrefixEntry {
    uint32_t prefix;
    int length;
    int next_hop;
    
    PrefixEntry(uint32_t p, int l, int nh) : prefix(p), length(l), next_hop(nh) {}
};

class ReferenceLPM {
private:
    std::vector<PrefixEntry> prefixes;
    
    bool matches(uint32_t address, uint32_t prefix, int length) {
        if (length == 0) return true;
        uint32_t mask = (length == 32) ? 0xFFFFFFFF : ((1U << length) - 1) << (32 - length);
        return (address & mask) == (prefix & mask);
    }
    
public:
    void insert(uint32_t prefix, int length, int next_hop) {
        prefixes.push_back(PrefixEntry(prefix, length, next_hop));
    }
    
    int lookup(uint32_t address) {
        int best_hop = -1;
        int best_length = -1;
        
        for (const auto& entry : prefixes) {
            if (matches(address, entry.prefix, entry.length)) {
                if (entry.length > best_length) {
                    best_length = entry.length;
                    best_hop = entry.next_hop;
                }
            }
        }
        
        return best_hop;
    }
    
    void clear() {
        prefixes.clear();
    }
    
    size_t size() const {
        return prefixes.size();
    }
};
