> Persian version: [README.fa.md](README.fa.md)

# Multibit Trie IP Lookup

A C++17 implementation of a multibit trie for longest-prefix-match (LPM) lookup of IP addresses, with a benchmark of strides 1, 2, 4 and 8.

## Prerequisites

- **C++17 compiler** (g++, clang++ or MSVC)
- **Python 3** with:
  - `pandas`
  - `matplotlib`
  - `numpy`

### Installing the Python libraries

```bash
pip install pandas matplotlib numpy
```

## Building

### Linux/macOS

```bash
g++ -std=c++17 -O2 -o trie_lookup main.cpp trie.cpp
```

### Windows (MSVC)

```cmd
cl /EHsc /std:c++17 /O2 main.cpp trie.cpp /Fe:trie_lookup.exe
```

### Windows (MinGW)

```bash
g++ -std=c++17 -O2 -o trie_lookup.exe main.cpp trie.cpp
```

## Usage

### Interactive mode

Run the program and use the CLI commands:

```bash
./trie_lookup
# or on Windows:
trie_lookup.exe
```

### Commands

```
build <stride>                          - build the trie from prefix-list.txt (stride: 1, 2, 4, 8)
insert <prefix_hex> <length> <next_hop> - insert a prefix manually
lookup <address>                        - look up one address (hex or decimal)
lookup-file <filename>                  - look up the addresses in a file
tprint                                  - print the trie structure
stats                                   - show lookup-time statistics
memory                                  - show memory statistics
save-stats <filename>                   - save the statistics to CSV
test-correctness <filename>             - check results against the reference method (20 addresses)
benchmark <filename>                    - run the benchmark for all strides (100,000 addresses)
quit / exit                             - quit
```

### Usage examples

The outputs below illustrate the format of each command; the measured results are in the [results summary](#results-summary).

#### 1. Build the trie from prefix-list.txt

```
> build 4
Built trie with stride 4 from prefix-list.txt (20000 prefixes inserted)
Node count: 15234
Estimated memory: 2437440 bytes
```

#### 2. Insert a prefix manually

```
> insert 40 13 1262
Inserted prefix: 40/13 -> next_hop=1262
```

#### 3. Look up one address

```
> lookup 0x40800000
Address: 0x40800000 -> next_hop=4513 (time: 45 ns)
```

#### 4. Look up addresses from a file

```
> lookup-file addresses.txt
0x40800000 -> 4513 (45 ns)
0x20440000 -> 2964 (38 ns)
...
Processed 100 addresses
```

#### 5. Print the trie structure

```
> tprint
Trie structure (stride=4):
root
  0 [next_hop=1262]
  4
    0 [next_hop=4513]
    8
      0 [next_hop=2964]
...
```

#### 6. Show statistics

```
> stats
Lookup Statistics:
  Count: 100000
  Min: 25 ns
  Max: 120 ns
  Average: 45.23 ns
  Std Dev: 12.45 ns
```

#### 7. Correctness test

First generate 20 test addresses:

```bash
python generate_test_addresses.py 20 correctness_test.txt
```

Then build the trie and run the test:

```
> build 4
> test-correctness correctness_test.txt

=== Correctness Test ===
Testing 20 addresses...
Correct: 20/20 (100.00%)
✓ All tests passed!
```

#### 8. Run the benchmark

First generate 100,000 test addresses:

```bash
python generate_test_addresses.py 100000 addresses.txt
```

Then run the benchmark:

```
> benchmark addresses.txt

=== Benchmark Mode ===
Testing strides: 1, 2, 4, 8
Using addresses from: addresses.txt

--- Testing stride 1 ---
Built trie with stride 1 from prefix-list.txt (20000 prefixes inserted)
...
Processed 100000 addresses
Statistics saved to results_stride_1.csv

--- Testing stride 2 ---
...

=== Benchmark Complete ===
Results saved to results_stride_*.csv files
```

## Report and results

### Full project report

The full report is in **`report.md`** (in Persian). It covers:

- an introduction to multibit tries
- design and implementation
- the main algorithms (insert, lookup, tprint)
- memory calculation
- correctness check (100% passed)
- performance results for strides 1, 2, 4 and 8
- detailed answers to questions A, B and C
- time and space complexity analysis
- conclusions and recommendations

### Result files

The benchmark writes the following files.

#### Summary CSV files

- `results_stride_1.csv`: results for stride 1
- `results_stride_2.csv`: results for stride 2
- `results_stride_4.csv`: results for stride 4
- `results_stride_8.csv`: results for stride 8

Each file contains: `stride, node_count, estimated_bytes, min_ns, max_ns, avg_ns, std_ns`.

#### Detailed CSV files

- `lookup_times_stride_1.csv`: lookup time of every address (100,000 addresses)
- `lookup_times_stride_2.csv`: lookup time of every address
- `lookup_times_stride_4.csv`: lookup time of every address
- `lookup_times_stride_8.csv`: lookup time of every address

**Note:** results are computed after removing outliers (0.13–0.23% of the data).

### Plots

To generate the plots:

```bash
python analyze.py
```

This produces:

- **`memory_vs_stride.png`**: memory use by stride
- **`avg_lookup_time_vs_stride.png`**: average lookup time
- **`lookup_time_stats.png`**: full statistics (min/max/avg ± standard deviation)
- **`node_count_vs_stride.png`**: number of nodes by stride

### Results summary

| Stride | Memory (MB) | Average time (ns) | Nodes | Throughput (M pps) |
|--------|-------------|-------------------|-----------|-------------------|
| 1      | 2.25        | 2,584.42          | 49,159    | 0.39              |
| 2      | 2.28        | 1,484.76          | 37,393    | 0.67              |
| 4      | 8.90        | 1,128.72          | 58,319    | 0.89              |
| 8      | 841.79      | 947.81            | 424,364   | 1.06              |

**Key findings:**

- Stride 2: the fewest nodes (37,393); the best choice when memory is limited
- Stride 4: the best overall balance between memory and speed
- Stride 8: the fastest, but with very high memory use (841.79 MB)

See **`report.md`** for details.

## File structure

```
.
├── main.cpp                      # main program and CLI
├── trie.h                        # MultibitTrie class header
├── trie.cpp                      # MultibitTrie implementation
├── reference.cpp                 # reference implementation for correctness checks
├── prefix-list.txt               # prefix input file (20,000 prefixes)
├── addresses.txt                 # test addresses (100,000)
├── correctness_test.txt          # correctness-test addresses (20)
├── generate_test_addresses.py    # generates test addresses
├── analyze.py                    # analysis and plotting script
├── test_trie_simulation.py       # Python simulation for testing
├── report.md                     # full project report (Persian)
├── README.md                     # this file
├── README.fa.md                  # Persian version of this file
│
├── results_stride_1.csv          # summary results, stride 1
├── results_stride_2.csv          # summary results, stride 2
├── results_stride_4.csv          # summary results, stride 4
├── results_stride_8.csv          # summary results, stride 8
│
├── lookup_times_stride_1.csv     # detailed times, stride 1
├── lookup_times_stride_2.csv     # detailed times, stride 2
├── lookup_times_stride_4.csv     # detailed times, stride 4
├── lookup_times_stride_8.csv     # detailed times, stride 8
│
├── memory_vs_stride.png          # memory plot
├── avg_lookup_time_vs_stride.png # average time plot
├── lookup_time_stats.png         # full time statistics plot
└── node_count_vs_stride.png      # node count plot
```

## `prefix-list.txt` format

Each line has 3 numbers:

```
prefix_hex length next_hop
```

Example:

```
40 13 1262
408 17 4513
2044 20 2964
```

- `prefix_hex`: the prefix in hexadecimal
- `length`: number of significant bits (0–32)
- `next_hop`: the next hop

## `addresses.txt` format

One address per line (hex or decimal):

```
0x40800000
0x20440000
1073741824
```

or without the `0x` prefix:

```
40800000
20440000
```

## Notes

1. **`prefix-list.txt` must be in the directory the program runs from.**
2. **For the correctness test, build the trie with `build` first.**
3. **Use 100,000 addresses for the benchmark so the results are reliable.**
4. **Times are reported in nanoseconds.**
5. **Results are computed after removing outliers (0.13–0.23% of the data).**

## Troubleshooting

### Error: "Cannot open file prefix-list.txt"

- Make sure `prefix-list.txt` is in the directory the program runs from.

### Error: "Stride must be 1, 2, 4, or 8"

- Only strides 1, 2, 4 and 8 are supported.

### Python error: "No module named 'pandas'"

- Install the Python libraries: `pip install pandas matplotlib numpy`

## Complete example

```bash
# 1. Build the project
g++ -std=c++17 -O2 -o trie_lookup main.cpp trie.cpp

# 2. Generate test addresses
python generate_test_addresses.py 20 correctness_test.txt
python generate_test_addresses.py 100000 addresses.txt

# 3. Run the program
./trie_lookup

# In the CLI:
> build 4
> test-correctness correctness_test.txt
> benchmark addresses.txt
> quit

# 4. Analyze the results and generate the plots
python analyze.py

# 5. Read the full report
# open report.md
```

## Author

Amirhossein Khoshbakht, February 2026
