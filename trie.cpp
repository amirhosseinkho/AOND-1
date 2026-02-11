#include "trie.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cmath>

MultibitTrie::MultibitTrie(int stride) : stride(stride), node_count(0) {
    if (stride != 1 && stride != 2 && stride != 4 && stride != 8) {
        throw std::invalid_argument("Stride must be 1, 2, 4, or 8");
    }
    root = new TrieNode(stride);
    node_count = 1;
}

MultibitTrie::~MultibitTrie() {
    if (root != nullptr) {
        delete root;
    }
}

uint32_t MultibitTrie::hexToInt(const std::string& hex_str) {
    uint32_t result = 0;
    std::istringstream iss(hex_str);
    iss >> std::hex >> result;
    return result;
}

int MultibitTrie::extractBits(uint32_t value, int start_bit, int num_bits) {
    uint32_t mask = (1U << num_bits) - 1;
    return (value >> (32 - start_bit - num_bits)) & mask;
}

void MultibitTrie::insert(uint32_t prefix, int length, int next_hop) {
    if (length < 0 || length > 32) {
        throw std::invalid_argument("Length must be between 0 and 32");
    }
    
    if (length == 0) {
        // Default route: keep the first (or shortest) if already set
        if (!root->has_prefix || root->prefix_length < 0) {
            root->next_hop = next_hop;
            root->has_prefix = true;
            root->prefix_length = 0;
        }
        return;
    }
    
    TrieNode* current = root;
    int bits_processed = 0;
    
    // Process full stride groups
    while (bits_processed + stride <= length) {
        int index = extractBits(prefix, bits_processed, stride);
        
        if (current->children[index] == nullptr) {
            current->children[index] = new TrieNode(stride);
            node_count++;
        }
        
        current = current->children[index];
        bits_processed += stride;
    }
    
    // Handle remaining bits if length is not a multiple of stride
    if (bits_processed < length) {
        int remaining_bits = length - bits_processed;
        // Extract the next stride bits (may include wildcard bits)
        int index = extractBits(prefix, bits_processed, stride);
        
        // Calculate mask for the remaining bits
        int mask_shift = stride - remaining_bits;
        int base_index = (index >> mask_shift) << mask_shift;
        int num_matching_children = 1 << mask_shift;
        
        // NOTE:
        // Do NOT store this longer prefix at the current node.
        // The current node represents only 'bits_processed' bits, اما
        // این prefix طولانی‌تر (length) فقط روی زیرمجموعه‌ای از این زیر‌درخت
        // معتبر است. اگر آن را اینجا ذخیره کنیم، تمام مسیرهای زیر این گره
        // (حتی آن‌هایی که در بیت‌های بعدی تطابق ندارند) به اشتباه این prefix
        // را به عنوان LPM خواهند دید.
        //
        // بنابراین، فقط روی فرزندان متناظر leaf-push انجام می‌دهیم تا
        // پوشش prefix دقیقاً با union زیر‌درخت‌های آن فرزندان منطبق باشد.
        
        // Push prefix to all matching children (leaf pushing)
        for (int i = 0; i < num_matching_children; i++) {
            int child_index = base_index | i;
            
            if (current->children[child_index] == nullptr) {
                current->children[child_index] = new TrieNode(stride);
                node_count++;
            }
            
            TrieNode* child = current->children[child_index];
            // Only override if this prefix is longer than any existing one at the child
            if (!child->has_prefix || length > child->prefix_length) {
                child->next_hop = next_hop;
                child->has_prefix = true;
                child->prefix_length = length;
            }
        }
    } else {
        // Exact match at stride boundary - store at current node
        if (!current->has_prefix || length > current->prefix_length) {
            current->next_hop = next_hop;
            current->has_prefix = true;
            current->prefix_length = length;
        }
    }
}

int MultibitTrie::lookup(uint32_t address) {
    int best_hop = -1;
    TrieNode* current = root;
    
    // Check root
    if (root->has_prefix) {
        best_hop = root->next_hop;
    }
    
    int bits_processed = 0;
    
    // Traverse the trie
    while (current != nullptr && bits_processed < 32) {
        int index = extractBits(address, bits_processed, stride);
        
        if (current->children[index] == nullptr) {
            break;
        }
        
        current = current->children[index];
        bits_processed += stride;
        
        // Update best match if this node has a prefix
        if (current->has_prefix) {
            best_hop = current->next_hop;
        }
    }
    
    return best_hop;
}

void MultibitTrie::printHelper(TrieNode* node, int depth, int path_value, const std::string& prefix_str) {
    if (node == nullptr) return;
    
    // Print current node
    std::string indent(depth * 2, ' ');
    std::string path_label;
    
    if (stride == 1) {
        path_label = (path_value == 0) ? "0" : "1";
    } else {
        path_label = std::to_string(path_value);
    }
    
    std::cout << indent << prefix_str << path_label;
    
    if (node->has_prefix) {
        std::cout << " [next_hop=" << node->next_hop << "]";
    }
    std::cout << std::endl;
    
    // Print children
    for (int i = 0; i < (1 << stride); i++) {
        if (node->children[i] != nullptr) {
            std::string new_prefix = prefix_str + path_label + (stride == 1 ? "/" : "-");
            printHelper(node->children[i], depth + 1, i, new_prefix);
        }
    }
}

void MultibitTrie::tprint() {
    std::cout << "Trie structure (stride=" << stride << "):" << std::endl;
    if (root->has_prefix) {
        std::cout << "root [next_hop=" << root->next_hop << "]" << std::endl;
    } else {
        std::cout << "root" << std::endl;
    }
    
    for (int i = 0; i < (1 << stride); i++) {
        if (root->children[i] != nullptr) {
            std::string prefix = (stride == 1) ? "" : "";
            printHelper(root->children[i], 1, i, prefix);
        }
    }
}

int MultibitTrie::getNodeCount() {
    return node_count;
}

void MultibitTrie::countNodesHelper(TrieNode* node) {
    if (node == nullptr) return;
    node_count++;
    for (TrieNode* child : node->children) {
        if (child != nullptr) {
            countNodesHelper(child);
        }
    }
}

size_t MultibitTrie::estimateMemory() {
    // Estimate memory per node:
    // - children vector: sizeof(vector) + (1 << stride) * sizeof(TrieNode*)
    // - next_hop: sizeof(int)
    // - has_prefix: sizeof(bool)
    // - prefix_length: sizeof(int)
    // - alignment overhead
    
    size_t children_size = sizeof(std::vector<TrieNode*>) + 
                          ((1 << stride) * sizeof(TrieNode*));
    size_t node_size = children_size + sizeof(int) + sizeof(bool) + sizeof(int);
    
    // Add alignment overhead (typically 8-byte alignment)
    node_size = ((node_size + 7) / 8) * 8;
    
    return node_count * node_size;
}

void MultibitTrie::resetStats() {
    // This could reset statistics if needed
    // For now, node_count is maintained during construction
}
