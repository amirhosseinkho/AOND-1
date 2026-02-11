# Multibit Trie IP Lookup Project

پیاده‌سازی درخت جستجوی چند بیتی (Multibit Trie) برای جستجوی Longest Prefix Match در آدرس‌های IP.

## پیش‌نیازها

- **C++17 compiler** (g++, clang++, یا MSVC)
- **Python 3** با کتابخانه‌های:
  - `pandas`
  - `matplotlib`
  - `numpy`

### نصب کتابخانه‌های Python

```bash
pip install pandas matplotlib numpy
```

## ساخت پروژه

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

## استفاده

### حالت تعاملی (Interactive Mode)

اجرای برنامه و استفاده از دستورات CLI:

```bash
./trie_lookup
# یا در Windows:
trie_lookup.exe
```

### دستورات موجود

```
build <stride>                    - ساخت trie از prefix-list.txt (stride: 1, 2, 4, 8)
insert <prefix_hex> <length> <next_hop> - درج یک prefix دستی
lookup <address>                   - جستجوی یک آدرس (hex یا decimal)
lookup-file <filename>            - جستجوی آدرس‌ها از فایل
tprint                             - چاپ ساختار درخت
stats                              - نمایش آمار زمان‌های lookup
memory                             - نمایش آمار حافظه
save-stats <filename>              - ذخیره آمار در CSV
test-correctness <filename>        - تست صحت با روش مرجع (20 آدرس)
benchmark <filename>               - اجرای benchmark برای تمام strideها (100000 آدرس)
quit / exit                        - خروج
```

### مثال‌های استفاده

#### 1. ساخت trie از فایل prefix-list.txt

```
> build 4
Built trie with stride 4 from prefix-list.txt (20000 prefixes inserted)
Node count: 15234
Estimated memory: 2437440 bytes
```

#### 2. درج دستی یک prefix

```
> insert 40 13 1262
Inserted prefix: 40/13 -> next_hop=1262
```

#### 3. جستجوی یک آدرس

```
> lookup 0x40800000
Address: 0x40800000 -> next_hop=4513 (time: 45 ns)
```

#### 4. جستجوی از فایل

```
> lookup-file addresses.txt
0x40800000 -> 4513 (45 ns)
0x20440000 -> 2964 (38 ns)
...
Processed 100 addresses
```

#### 5. چاپ ساختار درخت

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

#### 6. نمایش آمار

```
> stats
Lookup Statistics:
  Count: 100000
  Min: 25 ns
  Max: 120 ns
  Average: 45.23 ns
  Std Dev: 12.45 ns
```

#### 7. تست صحت

ابتدا 20 آدرس تست تولید کنید:

```bash
python generate_test_addresses.py 20 correctness_test.txt
```

سپس trie را بسازید و تست کنید:

```
> build 4
> test-correctness correctness_test.txt

=== Correctness Test ===
Testing 20 addresses...
Correct: 20/20 (100.00%)
✓ All tests passed!
```

#### 8. اجرای Benchmark

ابتدا 100000 آدرس تست تولید کنید:

```bash
python generate_test_addresses.py 100000 addresses.txt
```

سپس benchmark را اجرا کنید:

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

## تحلیل نتایج

پس از اجرای benchmark، فایل‌های CSV زیر تولید می‌شوند:

- `results_stride_1.csv`
- `results_stride_2.csv`
- `results_stride_4.csv`
- `results_stride_8.csv`
- `lookup_times_stride_*.csv` (زمان‌های تفصیلی)

برای تولید نمودارها:

```bash
python analyze.py
```

این دستور نمودارهای زیر را تولید می‌کند:

- `memory_vs_stride.png` - نمودار حافظه برحسب stride
- `avg_lookup_time_vs_stride.png` - نمودار زمان متوسط lookup
- `lookup_time_stats.png` - نمودار min/max/avg با error bar
- `node_count_vs_stride.png` - نمودار تعداد گره‌ها

## ساختار فایل‌ها

```
.
├── main.cpp                    # برنامه اصلی و CLI
├── trie.h                      # هدر کلاس MultibitTrie
├── trie.cpp                    # پیاده‌سازی MultibitTrie
├── prefix-list.txt             # فایل prefixها (ورودی)
├── generate_test_addresses.py  # تولید آدرس‌های تست
├── analyze.py                  # اسکریپت تحلیل و نمودار
├── report.md                   # گزارش کامل پروژه
└── README.md                   # این فایل
```

## فرمت فایل prefix-list.txt

هر خط شامل 3 عدد است:

```
prefix_hex length next_hop
```

مثال:
```
40 13 1262
408 17 4513
2044 20 2964
```

- `prefix_hex`: prefix در مبنای 16 (hex)
- `length`: تعداد بیت‌های معتبر (0-32)
- `next_hop`: گام بعدی (next hop)

## فرمت فایل addresses.txt

هر خط یک آدرس است (hex یا decimal):

```
0x40800000
0x20440000
1073741824
```

یا بدون پیشوند 0x:
```
40800000
20440000
```

## نکات مهم

1. **فایل prefix-list.txt باید در همان دایرکتوری اجرای برنامه باشد**
2. **برای تست صحت، ابتدا trie را با `build` بسازید**
3. **برای benchmark، از 100000 آدرس استفاده کنید تا نتایج قابل اعتماد باشند**
4. **زمان‌ها در نانوثانیه (nanoseconds) گزارش می‌شوند**

## عیب‌یابی

### خطا: "Cannot open file prefix-list.txt"
- مطمئن شوید فایل `prefix-list.txt` در همان دایرکتوری اجرای برنامه است

### خطا: "Stride must be 1, 2, 4, or 8"
- فقط strideهای 1، 2، 4، و 8 پشتیبانی می‌شوند

### خطا در Python: "No module named 'pandas'"
- کتابخانه‌های Python را نصب کنید: `pip install pandas matplotlib numpy`

## مثال کامل اجرا

```bash
# 1. ساخت پروژه
g++ -std=c++17 -O2 -o trie_lookup main.cpp trie.cpp

# 2. تولید آدرس‌های تست
python generate_test_addresses.py 20 correctness_test.txt
python generate_test_addresses.py 100000 addresses.txt

# 3. اجرای برنامه
./trie_lookup

# در CLI:
> build 4
> test-correctness correctness_test.txt
> benchmark addresses.txt
> quit

# 4. تحلیل نتایج
python analyze.py
```

## مجوز

این پروژه برای استفاده آموزشی تهیه شده است.

## نویسنده

[نام دانشجو]  
[تاریخ]
