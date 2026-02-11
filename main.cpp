#include "trie.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <chrono>
#include <iomanip>
#include <cmath>
#include <algorithm>

MultibitTrie* trie = nullptr;
std::vector<long long> lookup_times;  // in nanoseconds

// Reference implementation for correctness testing
struct PrefixEntry {
    uint32_t prefix;
    int length;
    int next_hop;
};

std::vector<PrefixEntry> reference_table;

// Reference LPM lookup using linear search
int reference_lpm_lookup(uint32_t address, const std::vector<PrefixEntry>& table) {
    int best_hop = -1;
    int best_length = -1;
    
    for (const auto& entry : table) {
        if (entry.length == 0) {
            // Default route
            if (best_length < 0) {
                best_hop = entry.next_hop;
                best_length = 0;
            }
            continue;
        }
        
        // Create mask for prefix (prefix is already left-aligned in the table)
        uint32_t mask = (entry.length == 32) ? 0xFFFFFFFF : (~0u << (32 - entry.length));
        uint32_t prefix_masked = entry.prefix & mask;
        uint32_t address_masked = address & mask;
        
        if (prefix_masked == address_masked && entry.length > best_length) {
            best_hop = entry.next_hop;
            best_length = entry.length;
        }
    }
    
    return best_hop;
}

uint32_t hexToUint32(const std::string& hex_str) {
    uint32_t result = 0;
    std::istringstream iss(hex_str);
    iss >> std::hex >> result;
    return result;
}

void buildTrie(const std::string& filename, int stride) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    if (trie != nullptr) {
        delete trie;
    }
    
    trie = new MultibitTrie(stride);
    reference_table.clear();  // Clear previous entries
    std::string line;
    int count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        std::istringstream iss(line);
        std::string prefix_hex;
        int length, next_hop;
        
        if (iss >> prefix_hex >> length >> next_hop) {
            // prefix_hex already denotes the prefix bits in a 32-bit space.
            // We parse it directly and use 'length' to limit the number of
            // significant bits when matching.
            uint32_t prefix = hexToUint32(prefix_hex);
            trie->insert(prefix, length, next_hop);
            // Store in reference table using the same raw prefix
            reference_table.push_back({prefix, length, next_hop});
            count++;
        }
    }
    
    file.close();
    std::cout << "Built trie with stride " << stride << " from " << filename 
              << " (" << count << " prefixes inserted)" << std::endl;
    std::cout << "Node count: " << trie->getNodeCount() << std::endl;
    std::cout << "Estimated memory: " << trie->estimateMemory() << " bytes" << std::endl;
}

void insertPrefix(const std::string& prefix_hex, int length, int next_hop) {
    if (trie == nullptr) {
        std::cerr << "Error: Trie not initialized. Use 'build <stride>' first." << std::endl;
        return;
    }
    
    uint32_t prefix = hexToUint32(prefix_hex);
    trie->insert(prefix, length, next_hop);
    // Also add to reference table so correctness tests include manual inserts
    reference_table.push_back({prefix, length, next_hop});
    std::cout << "Inserted prefix: " << prefix_hex << "/" << length 
              << " -> next_hop=" << next_hop << std::endl;
}

void lookupAddress(uint32_t address, bool record_time = true) {
    if (trie == nullptr) {
        std::cerr << "Error: Trie not initialized. Use 'build <stride>' first." << std::endl;
        return;
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    int next_hop = trie->lookup(address);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    std::cout << "Address: 0x" << std::hex << std::uppercase << address << std::dec
              << " -> next_hop=" << next_hop 
              << " (time: " << duration << " ns)" << std::endl;
    
    if (record_time) {
        lookup_times.push_back(duration);
    }
}

void lookupFromFile(const std::string& filename, bool verbose = true) {
    if (trie == nullptr) {
        std::cerr << "Error: Trie not initialized. Use 'build <stride>' first." << std::endl;
        return;
    }
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << filename << std::endl;
        return;
    }
    
    lookup_times.clear();
    std::string line;
    int count = 0;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        uint32_t address;
        // Try to parse as hex first
        if (line.find("0x") == 0 || line.find("0X") == 0) {
            address = hexToUint32(line.substr(2));
        } else {
            // Try as decimal
            std::istringstream iss(line);
            if (iss >> address) {
                // Assume it's already in correct format
            } else {
                // Try as hex without 0x
                address = hexToUint32(line);
            }
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        int next_hop = trie->lookup(address);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        lookup_times.push_back(duration);
        
        if (verbose) {
            std::cout << "0x" << std::hex << std::uppercase << address << std::dec
                      << " -> " << next_hop << " (" << duration << " ns)" << std::endl;
        }
        count++;
        
        // Progress indicator for large files
        if (!verbose && count % 10000 == 0) {
            std::cout << "Processed " << count << " addresses..." << std::endl;
        }
    }
    
    file.close();
    if (verbose) {
        std::cout << "\nProcessed " << count << " addresses" << std::endl;
    } else {
        std::cout << "Processed " << count << " addresses" << std::endl;
    }
}

void printStats() {
    if (lookup_times.empty()) {
        std::cout << "No lookup times recorded yet." << std::endl;
        return;
    }
    
    long long sum = 0;
    long long min_time = lookup_times[0];
    long long max_time = lookup_times[0];
    
    for (long long time : lookup_times) {
        sum += time;
        min_time = std::min(min_time, time);
        max_time = std::max(max_time, time);
    }
    
    double avg = static_cast<double>(sum) / lookup_times.size();
    
    double variance = 0.0;
    for (long long time : lookup_times) {
        double diff = time - avg;
        variance += diff * diff;
    }
    variance /= lookup_times.size();
    double std_dev = std::sqrt(variance);
    
    std::cout << "Lookup Statistics:" << std::endl;
    std::cout << "  Count: " << lookup_times.size() << std::endl;
    std::cout << "  Min: " << min_time << " ns" << std::endl;
    std::cout << "  Max: " << max_time << " ns" << std::endl;
    std::cout << "  Average: " << std::fixed << std::setprecision(2) << avg << " ns" << std::endl;
    std::cout << "  Std Dev: " << std::fixed << std::setprecision(2) << std_dev << " ns" << std::endl;
}

void printMemory() {
    if (trie == nullptr) {
        std::cerr << "Error: Trie not initialized. Use 'build <stride>' first." << std::endl;
        return;
    }
    
    int node_count = trie->getNodeCount();
    size_t estimated_bytes = trie->estimateMemory();
    
    std::cout << "Memory Statistics:" << std::endl;
    std::cout << "  Node count: " << node_count << std::endl;
    std::cout << "  Estimated memory: " << estimated_bytes << " bytes" << std::endl;
    std::cout << "  Estimated memory: " << std::fixed << std::setprecision(2) 
              << (estimated_bytes / 1024.0) << " KB" << std::endl;
    std::cout << "  Estimated memory: " << std::fixed << std::setprecision(2) 
              << (estimated_bytes / (1024.0 * 1024.0)) << " MB" << std::endl;
}

void saveStatsToCSV(const std::string& filename, int stride) {
    if (lookup_times.empty()) {
        std::cerr << "Error: No lookup times to save." << std::endl;
        return;
    }
    
    if (trie == nullptr) {
        std::cerr << "Error: Trie not initialized." << std::endl;
        return;
    }
    
    long long sum = 0;
    long long min_time = lookup_times[0];
    long long max_time = lookup_times[0];
    
    for (long long time : lookup_times) {
        sum += time;
        min_time = std::min(min_time, time);
        max_time = std::max(max_time, time);
    }
    
    double avg = static_cast<double>(sum) / lookup_times.size();
    
    double variance = 0.0;
    for (long long time : lookup_times) {
        double diff = time - avg;
        variance += diff * diff;
    }
    variance /= lookup_times.size();
    double std_dev = std::sqrt(variance);
    
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot create file " << filename << std::endl;
        return;
    }
    
    file << "stride,node_count,estimated_bytes,min_ns,max_ns,avg_ns,std_ns\n";
    file << stride << "," << trie->getNodeCount() << "," << trie->estimateMemory()
         << "," << min_time << "," << max_time << "," << std::fixed << std::setprecision(2)
         << avg << "," << std_dev << "\n";
    
    file.close();
    std::cout << "Statistics saved to " << filename << std::endl;
}

void testCorrectness(const std::string& addresses_file) {
    if (trie == nullptr) {
        std::cerr << "Error: Trie not initialized. Use 'build <stride>' first." << std::endl;
        return;
    }
    
    if (reference_table.empty()) {
        std::cerr << "Error: Reference table is empty. Build trie from file first." << std::endl;
        return;
    }
    
    std::ifstream file(addresses_file);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file " << addresses_file << std::endl;
        return;
    }
    
    std::vector<uint32_t> addresses;
    std::string line;
    
    while (std::getline(file, line)) {
        if (line.empty()) continue;
        
        uint32_t address;
        if (line.find("0x") == 0 || line.find("0X") == 0) {
            address = hexToUint32(line.substr(2));
        } else {
            std::istringstream iss(line);
            if (!(iss >> address)) {
                address = hexToUint32(line);
            }
        }
        addresses.push_back(address);
    }
    file.close();
    
    int correct = 0;
    int total = addresses.size();
    
    std::cout << "\n=== Correctness Test ===" << std::endl;
    std::cout << "Testing " << total << " addresses..." << std::endl;
    
    int debug_mismatches = 0;
    for (uint32_t addr : addresses) {
        int trie_result = trie->lookup(addr);
        int ref_result = reference_lpm_lookup(addr, reference_table);
        
        if (trie_result == ref_result) {
            correct++;
        } else {
            std::cout << "MISMATCH: Address 0x" << std::hex << std::uppercase << addr << std::dec
                      << " -> Trie: " << trie_result << ", Reference: " << ref_result << std::endl;
            
            // Extra debug for a few mismatches: show top matching prefixes
            if (debug_mismatches < 5) {
                struct MatchInfo {
                    int length;
                    int next_hop;
                    uint32_t prefix_val;
                    uint32_t masked_prefix;
                    uint32_t masked_addr;
                };
                std::vector<MatchInfo> matches;
                
                for (const auto& entry : reference_table) {
                    uint32_t p = entry.prefix;
                    int len = entry.length;
                    int nh = entry.next_hop;
                    
                    if (len <= 0) continue;
                    
                    uint32_t mask = (len == 32) ? 0xFFFFFFFFu : ((0xFFFFFFFFu << (32 - len)) & 0xFFFFFFFFu);
                    uint32_t p_masked = p & mask;
                    uint32_t a_masked = addr & mask;
                    
                    if (p_masked == a_masked) {
                        matches.push_back({len, nh, p, p_masked, a_masked});
                    }
                }
                
                std::sort(matches.begin(), matches.end(),
                          [](const MatchInfo& a, const MatchInfo& b) {
                              return a.length > b.length;
                          });
                
                std::cout << "  Top reference matches (length, next_hop, prefix_hex, addr_masked):" << std::endl;
                int limit = std::min<int>(3, matches.size());
                for (int i = 0; i < limit; ++i) {
                    const auto& m = matches[i];
                    std::cout << "    len=" << m.length
                              << ", nh=" << m.next_hop
                              << ", prefix=0x" << std::hex << std::uppercase << m.prefix_val
                              << ", masked_prefix=0x" << m.masked_prefix
                              << ", masked_addr=0x" << m.masked_addr
                              << std::dec << std::endl;
                }
                debug_mismatches++;
            }
        }
    }
    
    std::cout << "Correct: " << correct << "/" << total << " (" 
              << std::fixed << std::setprecision(2) 
              << (100.0 * correct / total) << "%)" << std::endl;
    
    if (correct == total) {
        std::cout << "✓ All tests passed!" << std::endl;
    } else {
        std::cout << "✗ Some tests failed!" << std::endl;
    }
}

void runBenchmark(const std::string& addresses_file) {
    std::cout << "\n=== Benchmark Mode ===" << std::endl;
    std::cout << "Testing strides: 1, 2, 4, 8" << std::endl;
    std::cout << "Using addresses from: " << addresses_file << std::endl;
    
    int strides[] = {1, 2, 4, 8};
    
    for (int s : strides) {
        std::cout << "\n--- Testing stride " << s << " ---" << std::endl;
        
        // Build trie
        if (trie != nullptr) {
            delete trie;
        }
        buildTrie("prefix-list.txt", s);
        
        if (trie == nullptr) {
            std::cerr << "Failed to build trie with stride " << s << std::endl;
            continue;
        }
        
        // Run lookups (silent mode for large files)
        lookup_times.clear();
        lookupFromFile(addresses_file, false);
        
        // Save stats
        std::ostringstream oss;
        oss << "results_stride_" << s << ".csv";
        saveStatsToCSV(oss.str(), s);
        
        // Also save detailed lookup times
        std::ostringstream oss2;
        oss2 << "lookup_times_stride_" << s << ".csv";
        std::ofstream detail_file(oss2.str());
        if (detail_file.is_open()) {
            detail_file << "lookup_time_ns\n";
            for (long long time : lookup_times) {
                detail_file << time << "\n";
            }
            detail_file.close();
            std::cout << "Detailed lookup times saved to " << oss2.str() << std::endl;
        }
    }
    
    std::cout << "\n=== Benchmark Complete ===" << std::endl;
    std::cout << "Results saved to results_stride_*.csv files" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string command;
    
    std::cout << "Multibit Trie IP Lookup - CLI" << std::endl;
    std::cout << "Type 'help' for available commands" << std::endl;
    
    while (true) {
        std::cout << "> ";
        std::getline(std::cin, command);
        
        if (command.empty()) continue;
        
        std::istringstream iss(command);
        std::string cmd;
        iss >> cmd;
        
        if (cmd == "help" || cmd == "h") {
            std::cout << "Available commands:" << std::endl;
            std::cout << "  build <stride>                    - Build trie from prefix-list.txt" << std::endl;
            std::cout << "  insert <prefix_hex> <length> <next_hop> - Insert a prefix" << std::endl;
            std::cout << "  lookup <address>                   - Lookup single address (hex or decimal)" << std::endl;
            std::cout << "  lookup-file <filename>            - Lookup addresses from file" << std::endl;
            std::cout << "  tprint                             - Print trie structure" << std::endl;
            std::cout << "  stats                              - Show lookup statistics" << std::endl;
            std::cout << "  memory                             - Show memory statistics" << std::endl;
            std::cout << "  save-stats <filename>              - Save statistics to CSV" << std::endl;
            std::cout << "  test-correctness <filename>        - Test correctness with reference (20 addresses)" << std::endl;
            std::cout << "  benchmark <filename>               - Run benchmark for all strides (100000 addresses)" << std::endl;
            std::cout << "  quit / exit                        - Exit program" << std::endl;
        }
        else if (cmd == "build") {
            int stride;
            if (iss >> stride) {
                if (stride == 1 || stride == 2 || stride == 4 || stride == 8) {
                    buildTrie("prefix-list.txt", stride);
                } else {
                    std::cerr << "Error: Stride must be 1, 2, 4, or 8" << std::endl;
                }
            } else {
                std::cerr << "Error: Usage: build <stride>" << std::endl;
            }
        }
        else if (cmd == "insert") {
            std::string prefix_hex;
            int length, next_hop;
            if (iss >> prefix_hex >> length >> next_hop) {
                insertPrefix(prefix_hex, length, next_hop);
            } else {
                std::cerr << "Error: Usage: insert <prefix_hex> <length> <next_hop>" << std::endl;
            }
        }
        else if (cmd == "lookup") {
            std::string addr_str;
            if (iss >> addr_str) {
                uint32_t address;
                if (addr_str.find("0x") == 0 || addr_str.find("0X") == 0) {
                    // Hex with 0x prefix
                    address = hexToUint32(addr_str.substr(2));
                } else {
                    // Try decimal first, then fall back to hex without 0x
                    std::istringstream iss_addr(addr_str);
                    if (!(iss_addr >> address)) {
                        address = hexToUint32(addr_str);
                    }
                }
                lookupAddress(address);
            } else {
                std::cerr << "Error: Usage: lookup <address>" << std::endl;
            }
        }
        else if (cmd == "lookup-file") {
            std::string filename;
            if (iss >> filename) {
                lookupFromFile(filename);
            } else {
                std::cerr << "Error: Usage: lookup-file <filename>" << std::endl;
            }
        }
        else if (cmd == "tprint") {
            if (trie == nullptr) {
                std::cerr << "Error: Trie not initialized. Use 'build <stride>' first." << std::endl;
            } else {
                trie->tprint();
            }
        }
        else if (cmd == "stats") {
            printStats();
        }
        else if (cmd == "memory") {
            printMemory();
        }
        else if (cmd == "save-stats") {
            std::string filename;
            if (iss >> filename) {
                if (trie == nullptr) {
                    std::cerr << "Error: Trie not initialized." << std::endl;
                } else {
                    // Determine stride from trie (we need to store it)
                    // For now, assume user knows the stride
                    int stride;
                    std::cout << "Enter stride: ";
                    std::cin >> stride;
                    std::cin.ignore();
                    saveStatsToCSV(filename, stride);
                }
            } else {
                std::cerr << "Error: Usage: save-stats <filename>" << std::endl;
            }
        }
        else if (cmd == "test-correctness") {
            std::string filename;
            if (iss >> filename) {
                testCorrectness(filename);
            } else {
                std::cerr << "Error: Usage: test-correctness <filename>" << std::endl;
            }
        }
        else if (cmd == "benchmark") {
            std::string filename;
            if (iss >> filename) {
                runBenchmark(filename);
            } else {
                std::cerr << "Error: Usage: benchmark <filename>" << std::endl;
            }
        }
        else if (cmd == "quit" || cmd == "exit" || cmd == "q") {
            if (trie != nullptr) {
                delete trie;
            }
            break;
        }
        else {
            std::cerr << "Unknown command: " << cmd << ". Type 'help' for available commands." << std::endl;
        }
    }
    
    return 0;
}
