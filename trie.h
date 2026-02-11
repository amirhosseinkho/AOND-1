#ifndef TRIE_H
#define TRIE_H

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

struct TrieNode {
    std::vector<TrieNode*> children;
    int next_hop;       // -1 if no prefix stored
    bool has_prefix;    // does this node store a prefix?
    int prefix_length;  // length of stored prefix in bits, -1 if none
    
    TrieNode(int stride) : next_hop(-1), has_prefix(false), prefix_length(-1) {
        children.resize(1 << stride, nullptr);
    }
    
    ~TrieNode() {
        for (TrieNode* child : children) {
            if (child != nullptr) {
                delete child;
            }
        }
    }
};

class MultibitTrie {
private:
    TrieNode* root;
    int stride;
    int node_count;
    
    // Helper functions
    uint32_t hexToInt(const std::string& hex_str);
    int extractBits(uint32_t value, int start_bit, int num_bits);
    void printHelper(TrieNode* node, int depth, int path_value, const std::string& prefix_str);
    void countNodesHelper(TrieNode* node);
    
public:
    MultibitTrie(int stride);
    ~MultibitTrie();
    
    // Main operations
    void insert(uint32_t prefix, int length, int next_hop);
    int lookup(uint32_t address);
    void tprint();
    
    // Statistics
    int getNodeCount();
    size_t estimateMemory();
    void resetStats();
};

#endif // TRIE_H
