#!/usr/bin/env python3
"""
Simulation of Multibit Trie for testing and generating sample results.
This simulates the C++ implementation to verify correctness and generate results.
"""

import struct
import random
import time
from collections import defaultdict

class TrieNode:
    def __init__(self, stride):
        self.children = [None] * (1 << stride)
        self.next_hop = -1
        self.has_prefix = False
        self.stride = stride

class MultibitTrie:
    def __init__(self, stride):
        if stride not in [1, 2, 4, 8]:
            raise ValueError("Stride must be 1, 2, 4, or 8")
        self.root = TrieNode(stride)
        self.stride = stride
        self.node_count = 1
    
    def extract_bits(self, value, start_bit, num_bits):
        """Extract num_bits starting from start_bit (MSB-first)."""
        mask = (1 << num_bits) - 1
        return (value >> (32 - start_bit - num_bits)) & mask
    
    def insert(self, prefix, length, next_hop):
        """Insert a prefix into the trie."""
        if length < 0 or length > 32:
            raise ValueError("Length must be between 0 and 32")
        
        if length == 0:
            self.root.next_hop = next_hop
            self.root.has_prefix = True
            return
        
        # Left-align prefix: shift to make it start from MSB, then mask to length
        if length > 0:
                    # Simple approach: shift prefix to start at bit position (32-length)
                    # This assumes prefix value represents the first 'length' bits
                    # But prefix might have fewer bits, so we left-align it first
                    if prefix == 0:
                        # Special case: all zeros
                        if length < 32:
                            mask = (0xFFFFFFFF << (32 - length)) & 0xFFFFFFFF
                            prefix = 0
                        else:
                            prefix = 0
                    else:
                        # Count significant bits in prefix value
                        actual_bits = prefix.bit_length()
                        # Left-align the prefix value to 32 bits
                        prefix_aligned = (prefix << (32 - actual_bits)) & 0xFFFFFFFF
                        # Now mask to the specified length
                        if length < 32:
                            mask = (0xFFFFFFFF << (32 - length)) & 0xFFFFFFFF
                            prefix = prefix_aligned & mask
                        else:
                            prefix = prefix_aligned
        
        current = self.root
        bits_processed = 0
        
        # Process full stride groups
        while bits_processed + self.stride <= length:
            index = self.extract_bits(prefix, bits_processed, self.stride)
            
            if current.children[index] is None:
                current.children[index] = TrieNode(self.stride)
                self.node_count += 1
            
            current = current.children[index]
            bits_processed += self.stride
        
        # Handle remaining bits if length is not a multiple of stride
        if bits_processed < length:
            remaining_bits = length - bits_processed
            index = self.extract_bits(prefix, bits_processed, self.stride)
            
            # Calculate mask for the remaining bits
            mask_shift = self.stride - remaining_bits
            base_index = (index >> mask_shift) << mask_shift
            num_matching_children = 1 << mask_shift
            
            # Store prefix at current node (shorter match)
            # This ensures we have a match even if we don't go deeper
            if not current.has_prefix or bits_processed > 0:
                current.next_hop = next_hop
                current.has_prefix = True
            
            # Push prefix to all matching children (leaf pushing)
            for i in range(num_matching_children):
                child_index = base_index | i
                
                if current.children[child_index] is None:
                    current.children[child_index] = TrieNode(self.stride)
                    self.node_count += 1
                
                # Always update to allow overriding with more specific prefixes
                current.children[child_index].next_hop = next_hop
                current.children[child_index].has_prefix = True
        else:
            # Exact match at stride boundary
            current.next_hop = next_hop
            current.has_prefix = True
    
    def lookup(self, address):
        """Lookup with LPM."""
        best_hop = -1
        current = self.root
        
        # Check root
        if self.root.has_prefix:
            best_hop = self.root.next_hop
        
        bits_processed = 0
        
        # Traverse the trie
        while current is not None and bits_processed < 32:
            index = self.extract_bits(address, bits_processed, self.stride)
            
            if current.children[index] is None:
                break
            
            current = current.children[index]
            bits_processed += self.stride
            
            # Update best match if this node has a prefix
            if current.has_prefix:
                best_hop = current.next_hop
        
        return best_hop
    
    def estimate_memory(self):
        """Estimate memory usage."""
        # Size per node: vector overhead + children pointers + next_hop + has_prefix
        children_size = 24 + ((1 << self.stride) * 8)  # 24 bytes vector overhead, 8 bytes per pointer
        node_size = children_size + 4 + 1  # int + bool
        # 8-byte alignment
        node_size = ((node_size + 7) // 8) * 8
        return self.node_count * node_size

def hex_to_int(hex_str):
    """Convert hex string to int."""
    return int(hex_str, 16)

def reference_lpm_lookup(address, table):
    """Reference LPM using linear search."""
    best_hop = -1
    best_length = -1
    
    for entry in table:
        prefix, length, next_hop = entry
        if length == 0:
            if best_length < 0:
                best_hop = next_hop
                best_length = 0
            continue
        
        mask = 0xFFFFFFFF if length == 32 else ((1 << 32) - 1) << (32 - length)
        prefix_masked = prefix & mask
        address_masked = address & mask
        
        if prefix_masked == address_masked and length > best_length:
            best_hop = next_hop
            best_length = length
    
    return best_hop

def load_prefixes(filename):
    """Load prefixes from file."""
    table = []
    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            parts = line.split()
            if len(parts) >= 3:
                prefix_hex = parts[0]
                length = int(parts[1])
                next_hop = int(parts[2])
                prefix = hex_to_int(prefix_hex)
                
                # Left-align prefix: same logic as in insert
                if length > 0:
                    if prefix == 0:
                        if length < 32:
                            mask = (0xFFFFFFFF << (32 - length)) & 0xFFFFFFFF
                            prefix = 0
                        else:
                            prefix = 0
                    else:
                        actual_bits = prefix.bit_length()
                        prefix_aligned = (prefix << (32 - actual_bits)) & 0xFFFFFFFF
                        if length < 32:
                            mask = (0xFFFFFFFF << (32 - length)) & 0xFFFFFFFF
                            prefix = prefix_aligned & mask
                        else:
                            prefix = prefix_aligned
                
                table.append((prefix, length, next_hop))
    return table

def test_correctness(stride, prefix_file, test_file):
    """Test correctness with reference implementation."""
    print(f"\n=== Testing Correctness (Stride {stride}) ===")
    
    # Load prefixes
    table = load_prefixes(prefix_file)
    print(f"Loaded {len(table)} prefixes")
    
    # Build trie
    trie = MultibitTrie(stride)
    for prefix, length, next_hop in table:
        trie.insert(prefix, length, next_hop)
    
    print(f"Trie built: {trie.node_count} nodes, {trie.estimate_memory()} bytes")
    
    # Load test addresses
    addresses = []
    with open(test_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith('0x') or line.startswith('0X'):
                addr = hex_to_int(line[2:])
            else:
                addr = hex_to_int(line)
            addresses.append(addr)
    
    print(f"Testing {len(addresses)} addresses...")
    
    # Test
    correct = 0
    for i, addr in enumerate(addresses):
        trie_result = trie.lookup(addr)
        ref_result = reference_lpm_lookup(addr, table)
        
        if trie_result == ref_result:
            correct += 1
        else:
            print(f"MISMATCH: 0x{addr:08X} -> Trie: {trie_result}, Reference: {ref_result}")
            # Debug: find matching prefixes
            if i < 2:  # Only for first 2 mismatches
                matches = []
                for p, l, nh in table:
                    if l > 0:
                        mask = 0xFFFFFFFF if l == 32 else ((1 << 32) - 1) << (32 - l)
                        if (addr & mask) == (p & mask):
                            matches.append((l, nh, hex(p), hex(addr & mask)))
                matches.sort(reverse=True)
                print(f"  Reference matches: {matches[:3]}")
    
    print(f"Correct: {correct}/{len(addresses)} ({100.0 * correct / len(addresses):.2f}%)")
    return correct == len(addresses)

def benchmark(stride, prefix_file, addresses_file):
    """Run benchmark for a stride."""
    print(f"\n--- Benchmarking Stride {stride} ---")
    
    # Load and build
    table = load_prefixes(prefix_file)
    trie = MultibitTrie(stride)
    for prefix, length, next_hop in table:
        trie.insert(prefix, length, next_hop)
    
    # Load addresses
    addresses = []
    with open(addresses_file, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            if line.startswith('0x') or line.startswith('0X'):
                addr = hex_to_int(line[2:])
            else:
                addr = hex_to_int(line)
            addresses.append(addr)
    
    # Run lookups with timing
    lookup_times = []
    for addr in addresses:
        start = time.perf_counter_ns()
        trie.lookup(addr)
        end = time.perf_counter_ns()
        lookup_times.append(end - start)
    
    # Calculate statistics
    min_time = min(lookup_times)
    max_time = max(lookup_times)
    avg_time = sum(lookup_times) / len(lookup_times)
    variance = sum((t - avg_time) ** 2 for t in lookup_times) / len(lookup_times)
    std_time = variance ** 0.5
    
    node_count = trie.node_count
    estimated_bytes = trie.estimate_memory()
    
    print(f"Node count: {node_count:,}")
    print(f"Estimated memory: {estimated_bytes:,} bytes ({estimated_bytes / (1024*1024):.2f} MB)")
    print(f"Min time: {min_time:.2f} ns")
    print(f"Max time: {max_time:.2f} ns")
    print(f"Avg time: {avg_time:.2f} ns")
    print(f"Std dev: {std_time:.2f} ns")
    
    # Save to CSV
    import csv
    filename = f"results_stride_{stride}.csv"
    with open(filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['stride', 'node_count', 'estimated_bytes', 'min_ns', 'max_ns', 'avg_ns', 'std_ns'])
        writer.writerow([stride, node_count, estimated_bytes, min_time, max_time, f"{avg_time:.2f}", f"{std_time:.2f}"])
    
    print(f"Results saved to {filename}")
    
    # Also save detailed lookup times
    detail_filename = f"lookup_times_stride_{stride}.csv"
    with open(detail_filename, 'w', newline='') as f:
        writer = csv.writer(f)
        writer.writerow(['lookup_time_ns'])
        for t in lookup_times:
            writer.writerow([t])
    print(f"Detailed lookup times saved to {detail_filename}")
    
    return {
        'stride': stride,
        'node_count': node_count,
        'estimated_bytes': estimated_bytes,
        'min_ns': min_time,
        'max_ns': max_time,
        'avg_ns': avg_time,
        'std_ns': std_time
    }

if __name__ == "__main__":
    import sys
    
    if len(sys.argv) > 1 and sys.argv[1] == "test":
        # Correctness test
        test_correctness(4, "prefix-list.txt", "correctness_test.txt")
    elif len(sys.argv) > 1 and sys.argv[1] == "benchmark":
        # Full benchmark
        print("=== Running Benchmark ===")
        results = []
        for stride in [1, 2, 4, 8]:
            result = benchmark(stride, "prefix-list.txt", "addresses.txt")
            results.append(result)
        print("\n=== Benchmark Complete ===")
    else:
        print("Usage:")
        print("  python test_trie_simulation.py test       - Run correctness test")
        print("  python test_trie_simulation.py benchmark   - Run full benchmark")
