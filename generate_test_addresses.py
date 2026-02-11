#!/usr/bin/env python3
"""
Generate random IPv4 destination addresses for testing the Multibit Trie.

Usage:
    python generate_test_addresses.py <count> <output_file>

Examples:
    # 20 addresses for correctness testing
    python generate_test_addresses.py 20 correctness_test.txt

    # 100000 addresses for performance benchmarking
    python generate_test_addresses.py 100000 addresses.txt

Addresses are generated uniformly at random over the 32‑bit space and
written one per line in hexadecimal form (0xXXXXXXXX). The C++ CLI
also accepts decimal, but hex is convenient and unambiguous.
"""

import sys
import random


def main() -> int:
    if len(sys.argv) != 3:
        print("Usage: python generate_test_addresses.py <count> <output_file>")
        return 1

    try:
        count = int(sys.argv[1])
        if count <= 0:
            raise ValueError
    except ValueError:
        print("Error: <count> must be a positive integer.")
        return 1

    output_file = sys.argv[2]

    # Fixed seed for reproducibility of experiments
    rng = random.Random(42)

    try:
        with open(output_file, "w", encoding="utf-8") as f:
            for _ in range(count):
                addr = rng.getrandbits(32)
                f.write(f"0x{addr:08X}\n")
    except OSError as e:
        print(f"Error: cannot write to {output_file}: {e}")
        return 1

    print(f"Generated {count} random addresses into {output_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

#!/usr/bin/env python3
"""
Generate test addresses for correctness and performance testing.
"""

import random
import sys

def generate_addresses(count, output_file):
    """Generate random 32-bit addresses with uniform distribution."""
    random.seed(42)  # For reproducibility
    addresses = []
    
    for _ in range(count):
        # Generate random 32-bit address
        address = random.randint(0, 0xFFFFFFFF)
        addresses.append(address)
    
    # Write to file (one per line, in hex format)
    with open(output_file, 'w') as f:
        for addr in addresses:
            f.write(f"0x{addr:08X}\n")
    
    print(f"Generated {count} addresses and saved to {output_file}")
    return addresses

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python generate_test_addresses.py <count> <output_file>")
        print("Example: python generate_test_addresses.py 20 correctness_test.txt")
        print("Example: python generate_test_addresses.py 100000 addresses.txt")
        sys.exit(1)
    
    count = int(sys.argv[1])
    output_file = sys.argv[2]
    
    generate_addresses(count, output_file)
