# AuctionHub Sorting Engine — Technical Reflection

**Course / Lab:** Data Structures & Algorithms  
**Project:** AuctionHub Real-World Sorting System  
**Implementation:** `auctionhub.cpp`  
**Dataset:** `test_data/small.txt` (4 records), `test_data/large.txt` (100,000 records)  

---

## 1. Algorithm Justifications by Real-World Module

### Module A: Live Auction Monitor — **Insertion Sort**
* **Technical Justification:** In a real-time auction stream, new bids are placed incrementally on top of the current highest price, meaning incoming data arrives in a *nearly-sorted* state with very few inversions ($d \ll N$). Insertion Sort is adaptive with a time complexity of $\mathcal{O}(N + d)$, reducing to asymptotically optimal $\mathcal{O}(N)$ linear time under these stream conditions. Additionally, it operates strictly in-place with zero memory allocation and maintains data stability for identical price bids.

### Module B: Top-K Bid Finder — **Selection Sort**
* **Technical Justification:** When extracting the top $k$ highest bids (e.g., $k=3$) from a bidding batch, Selection Sort naturally surfaces the $i$-th maximum element into its final sorted position on the $i$-th pass. Running $k$ selection passes requires only $\mathcal{O}(k \cdot N)$ comparisons without requiring auxiliary heap or tree allocations. Crucially, Selection Sort guarantees at most $\mathcal{O}(N)$ memory write/swap operations, which is optimal in hardware environments where memory writes are substantially more expensive than reads.

### Module C: Price Anomaly Validator — **Interchange Sort**
* **Technical Justification:** For high-integrity QA validation on small micro-batches ($N \le 50$), code readability, transparent formal verification, and zero risk of stack overflow are prioritized over asymptotic speed. Interchange Sort provides the simplest nested-loop structure, making it trivial to audit and debug. Furthermore, the total number of swaps executed directly quantifies the disorder and anomaly magnitude of the incoming batch.

### Module D: Stabilization Detector — **Bubble Sort (with Early Stop)**
* **Technical Justification:** The system needs to detect whether a streaming price window has ceased fluctuating without incurring sorting overhead on stable streams. Bubble Sort with an early-stop flag scans adjacent pairs in a single linear pass ($\mathcal{O}(N)$); if zero swaps occur during this initial pass, it proves the window is already settled and exits immediately. This provides an instant "STABLE" signal at minimal computational cost.

### Module E: Full Historical Sorter — **Quick Sort (Median-of-Three)**
* **Technical Justification:** Processing millions of historical bids for end-of-day audit reports demands an algorithm with high throughput and $\mathcal{O}(N \log N)$ average-case complexity. Quick Sort achieves exceptional speed due to tight inner loops and cache-friendly sequential memory access patterns. Using a **median-of-three** pivot (evaluating `first`, `middle`, and `last` elements) protects against the worst-case degradation typically provoked by already-sorted or reversed chronological logs.

---

## 2. Quick Sort Runtime Measurement on 100,000 Records

* **Dataset:** 100,000 randomized `Bid` records generated via `std::mt19937_64` (`test_data/large.txt`).
* **Environment:** GCC 16.1.0 (MSYS2 x86_64), C++17, `-O2` optimization.
* **Timing Mechanism:** `std::chrono::high_resolution_clock`.

### Benchmark Results
| Metric | Value |
|---|---|
| **Total Records Sorted** | 100,000 records |
| **Elapsed Time (Milliseconds)** | **36 ms** |
| **Elapsed Time (Microseconds)** | **36,330 µs** |
| **Average Time per Record** | ~0.36 µs |
| **Sorted Validation (`isSortedAscending`)** | **PASS [OK]** (100% verified) |

```
>>> [Task E] Quick Sort (Full Historical Sorter - 100,000 bids)
Loaded 100000 records from test_data/large.txt.
Starting QuickSort with Median-of-Three pivot...
QuickSort completed in: 36 ms (36330 us)
Validation (100,000 records strictly sorted): PASS [OK]
```

---

## 3. Bug Encounter & Diagnostic Walkthrough

### Bug Description
During the initial implementation of Task E (Quick Sort) and Task A (Insertion Sort), sorting records with identical `amount` values produced inconsistent orderings between runs. Specifically, bids with equal dollar amounts were either reordered unpredictably or caused unnecessary element swaps during partitioning.

### Root Cause Diagnosis
The comparator logic initially evaluated floating-point equality using `>=` and `<=` directly in the partitioning loop:
```cpp
// Flawed initial check in partition:
if (v[j].amount <= pivot.amount) { ... }
```
Because `amount` is a floating-point `double`, and `Bid` contains a secondary tie-breaker (`timestamp`), this raw inequality lacked strict weak ordering. When multiple bids shared the same amount, their relative arrival times (`timestamp`) were ignored by the partitioner. Furthermore, the median-of-three selection function was not using the exact same tie-breaker logic, causing the pivot selection and partition scanning to disagree on element ordering.

### Fix & Resolution
1. Created a unified strict weak ordering predicate `bidLess`:
   ```cpp
   bool bidLess(const Bid& a, const Bid& b) {
       if (a.amount != b.amount)
           return a.amount < b.amount;
       return a.timestamp < b.timestamp;
   }
   ```
2. Rewrote the partition condition to check `!bidLess(pivot, v[j])` (meaning $v[j] \le \text{pivot}$ under the exact same multi-key rule).
3. Standardized `getMedianIndex` to evaluate all three candidate indices (`lo`, `mid`, `hi`) using `bidLess`.
4. Re-ran validation on edge cases with duplicate prices; the verification test passed with 100% deterministic ordering.

---

## 4. LLM Prompts & Compliance Classification

| # | Prompt Sent to LLM | Policy Zone | Justification |
|---|---|---|---|
| **1** | *"Explain why insertion sort is O(N) on nearly-sorted data and how the count of inversions impacts its outer vs inner loop execution."* | **Green Zone** (Allowed) | Inquires about theoretical algorithm dynamics and complexity analysis. Does not request code or implementation for the assignment. |
| **2** | *"What is the standard branchless or minimal-comparison idiom in C++ to find the median of three values?"* | **Yellow Zone** (Allowed with Caution) | Inquires about an isolated algorithmic technique/idiom. The student still had to translate the concept to `Bid` structs, integrate tie-breakers, and write the full sorting logic. |
| **3** | *"Write the full bubbleSortEarlyStop function for my AuctionHub assignment that returns true if no swaps occurred on the first pass."* | **Red Zone** (Not Allowed) | Directly requests complete, assignment-specific production code ("ghostwriting"). This bypasses student implementation and violates the academic integrity policy. |

---

## 5. Worst-Case Complexity Analysis of Quick Sort with Median-of-Three

### Question
> *State the worst-case complexity of Quick Sort with median-of-three pivot. Is it still $\mathcal{O}(n^2)$? Why or why not?*

### Theoretical Analysis
* **Worst-Case Complexity:** **$\mathcal{O}(n^2)$** (Quadratic time).
* **Is it still $\mathcal{O}(n^2)$?** **Yes.**

### Mathematical & Algorithmic Explanation
While median-of-three pivot selection successfully protects against the canonical worst-case scenarios of naive Quick Sort (such as pre-sorted or reverse-sorted arrays where selecting `arr[0]` or `arr[n-1]` produces partitions of size $0$ and $n-1$), it **does not eliminate the theoretical $\mathcal{O}(n^2)$ worst case**.

1. **Adversarial Sequences (Median-of-Three Killer Permutations):**  
   For any deterministic pivot selection rule, an adversary can construct an input sequence where the median of `v[lo]`, `v[mid]`, and `v[hi]` consistently ends up being the second-smallest (or second-largest) element in the active sub-array. A well-known construction is the *Musser permutation* (or McIntyre & Snoeyink's adversary).

2. **Degenerate Recurrence Relation:**  
   When the chosen median repeatedly splits an array of size $k$ into subproblems of size $1$ and $k - 2$, the recursion tree reaches depth $\mathcal{O}(n)$:
   $$T(n) = T(n - 2) + T(1) + \mathcal{O}(n) = T(n - 2) + \mathcal{O}(n)$$
   Expanding the summation:
   $$T(n) = \sum_{i=1}^{n/2} \mathcal{O}(2i) = \mathcal{O}(n^2)$$

3. **Production Mitigations:**  
   Because deterministic median-of-three still allows $\mathcal{O}(n^2)$ worst-case behavior, industrial standard libraries (such as C++ `std::sort`) implement **Introsort** (Introspective Sort): they begin with Quick Sort (using median-of-three), track the recursion depth, and automatically switch to **Heap Sort** if the recursion depth exceeds $2 \lfloor \log_2 n \rfloor$, guaranteeing a hard $\mathcal{O}(n \log n)$ worst-case bound.
