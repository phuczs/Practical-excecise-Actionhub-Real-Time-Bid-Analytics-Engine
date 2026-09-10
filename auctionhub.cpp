#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <chrono>
#include <algorithm>
#include <iomanip>

using namespace std;
using namespace std::chrono;

struct Bid {
    string bidderId;
    double amount;       // bid price
    long long timestamp; // Unix ms
    int auctionId;
};

// ---------- Utilities ----------
void printBids(const vector<Bid>& v, size_t limit = 10) {
    for (size_t i = 0; i < min(v.size(), limit); ++i)
        cout << "[" << v[i].bidderId << "] $" << fixed << setprecision(2) << v[i].amount
             << " @ " << v[i].timestamp << " (Auction #" << v[i].auctionId << ")\n";
    if (v.size() > limit) cout << "... (" << v.size() << " total)\n";
}

bool loadBids(const string& path, vector<Bid>& out) {
    ifstream f(path);
    if (!f) return false;
    Bid b;
    while (f >> b.bidderId >> b.amount >> b.timestamp >> b.auctionId)
        out.push_back(b);
    return true;
}

// Comparison predicate: Ascending by amount; tie-break by timestamp ascending
bool bidLess(const Bid& a, const Bid& b) {
    if (a.amount != b.amount)
        return a.amount < b.amount;
    return a.timestamp < b.timestamp;
}

// Comparison predicate: Descending by amount; tie-break by timestamp ascending (earlier bid wins)
bool bidGreater(const Bid& a, const Bid& b) {
    if (a.amount != b.amount)
        return a.amount > b.amount;
    return a.timestamp < b.timestamp;
}

// Verification utility
bool isSortedAscending(const vector<Bid>& v) {
    for (size_t i = 1; i < v.size(); ++i) {
        if (bidLess(v[i], v[i - 1]))
            return false;
    }
    return true;
}

// ---------- Task A: Insertion Sort (Live Auction Monitor) ----------
// Real-world justification: For a live stream where new bids arrive in nearly-sorted order,
// Insertion Sort executes in O(N + d) time (where d is the count of inversions).
// When d is minimal, it operates in near O(N) linear time with minimal overhead and zero allocations.
void insertionSort(vector<Bid>& v) {
    int n = static_cast<int>(v.size());
    for (int i = 1; i < n; ++i) {
        Bid key = v[i];
        int j = i - 1;
        // Shift elements greater than key to the right
        while (j >= 0 && bidLess(key, v[j])) {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = key;
    }
}

// ---------- Task B: Selection Sort (Top-K Bid Finder) ----------
// Real-world justification: When searching for top-K maximum bids in small sets,
// Selection Sort directly surfaces the k-th extreme value in exactly k passes without
// extra memory overhead (in-place) and minimizes total data writes to O(N).
void selectionSort(vector<Bid>& v) {
    int n = static_cast<int>(v.size());
    for (int i = 0; i < n; ++i) {
        int maxIdx = i;
        for (int j = i + 1; j < n; ++j) {
            if (bidGreater(v[j], v[maxIdx])) {
                maxIdx = j;
            }
        }
        if (maxIdx != i) {
            std::swap(v[i], v[maxIdx]);
        }
    }
}

// ---------- Task C: Interchange Sort (Price Anomaly Validator) ----------
// Real-world justification: In mission-critical QA validators inspecting small batches (<= 50 records),
// Interchange Sort offers unmatched implementation simplicity and zero hidden recursion or edge-case bugs.
// Its quadratic complexity is negligible for N <= 50, and swap count directly reflects stream disorder.
void interchangeSort(vector<Bid>& v) {
    int n = static_cast<int>(v.size());
    long long swapCount = 0;
    for (int i = 0; i < n - 1; ++i) {
        for (int j = i + 1; j < n; ++j) {
            if (bidLess(v[j], v[i])) {
                std::swap(v[i], v[j]);
                swapCount++;
            }
        }
    }
    cout << "  [Interchange Sort] Total swaps performed: " << swapCount << "\n";
}

// ---------- Task D: Bubble Sort with Early Stop (Stabilization Detector) ----------
// Real-world justification: Bubble Sort with early stop can detect whether an incoming price
// stream is already stabilized in a single O(N) pass if 0 swaps are triggered, avoiding
// expensive sorting overhead for stable windows.
bool bubbleSortEarlyStop(vector<Bid>& v) {
    int n = static_cast<int>(v.size());
    if (n <= 1) return true;

    bool stableOnFirstPass = true;
    for (int i = 0; i < n - 1; ++i) {
        bool swapped = false;
        for (int j = 0; j < n - 1 - i; ++j) {
            if (bidLess(v[j + 1], v[j])) {
                std::swap(v[j], v[j + 1]);
                swapped = true;
            }
        }
        if (i == 0 && swapped) {
            stableOnFirstPass = false;
        }
        if (!swapped) {
            break; // Early stop: collection is sorted
        }
    }
    return stableOnFirstPass;
}

// ---------- Task E: Quick Sort (Full Historical Sorter) ----------
// Real-world justification: For millions of historical records, Quick Sort delivers average O(N log N)
// performance with excellent CPU cache locality and in-place partitioning. Median-of-three pivot selection
// protects against pathological O(N^2) degradation on sorted or reversed data.

// Helper: Select index of median among v[lo], v[mid], v[hi]
int getMedianIndex(const vector<Bid>& v, int lo, int hi) {
    int mid = lo + (hi - lo) / 2;
    if (bidLess(v[lo], v[mid])) {
        // v[lo] < v[mid]
        if (bidLess(v[mid], v[hi])) return mid; // lo < mid < hi
        if (bidLess(v[lo], v[hi]))  return hi;  // lo < hi <= mid
        return lo;                              // hi <= lo < mid
    } else {
        // v[mid] <= v[lo]
        if (bidLess(v[lo], v[hi]))  return lo;  // mid <= lo < hi
        if (bidLess(v[mid], v[hi])) return hi;  // mid < hi <= lo
        return mid;                             // hi <= mid <= lo
    }
}

// Lomuto partition scheme with Median-of-Three pivot
int partition(vector<Bid>& v, int lo, int hi) {
    int medianIdx = getMedianIndex(v, lo, hi);
    std::swap(v[medianIdx], v[hi]); // Place chosen pivot at hi

    const Bid& pivot = v[hi];
    int i = lo - 1;
    for (int j = lo; j < hi; ++j) {
        // If v[j] <= pivot (i.e., !(pivot < v[j]))
        if (!bidLess(pivot, v[j])) {
            i++;
            std::swap(v[i], v[j]);
        }
    }
    std::swap(v[i + 1], v[hi]);
    return i + 1;
}

void quickSort(vector<Bid>& v, int lo, int hi) {
    if (lo < hi) {
        int pi = partition(v, lo, hi);
        quickSort(v, lo, pi - 1);
        quickSort(v, pi + 1, hi);
    }
}

void quickSortWrapper(vector<Bid>& v) {
    if (v.size() > 1) {
        quickSort(v, 0, static_cast<int>(v.size()) - 1);
    }
}

// ---------- Driver ----------
int main(int argc, char* argv[]) {
    cout << "=====================================================\n";
    cout << "       AuctionHub Analytics Engine - Sorting Suite    \n";
    cout << "=====================================================\n\n";

    // --------------------------------------------------
    // Task A: Insertion Sort (Live Auction Monitor)
    // --------------------------------------------------
    cout << ">>> [Task A] Insertion Sort (Live Auction Monitor)\n";
    vector<Bid> bidsA;
    if (!loadBids("test_data/small.txt", bidsA)) {
        cerr << "Error: Could not load test_data/small.txt\n";
        return 1;
    }
    cout << "Original stream:\n";
    printBids(bidsA);

    insertionSort(bidsA);

    cout << "\nSorted stream (Ascending by amount, tie-break timestamp):\n";
    printBids(bidsA);
    cout << "Verification: " << (isSortedAscending(bidsA) ? "PASS [OK]" : "FAIL") << "\n\n";

    // --------------------------------------------------
    // Task B: Selection Sort (Top-K Bid Finder)
    // --------------------------------------------------
    cout << ">>> [Task B] Selection Sort (Top-3 Bid Finder)\n";
    vector<Bid> bidsB;
    loadBids("test_data/small.txt", bidsB);
    selectionSort(bidsB);

    cout << "Top 3 Bidders (Highest Amount):\n";
    for (size_t i = 0; i < min(bidsB.size(), (size_t)3); ++i) {
        cout << "  #" << (i + 1) << ": [" << bidsB[i].bidderId << "] $"
             << bidsB[i].amount << " @ " << bidsB[i].timestamp << "\n";
    }
    cout << "\n";

    // --------------------------------------------------
    // Task C: Interchange Sort (Price Anomaly Validator)
    // --------------------------------------------------
    cout << ">>> [Task C] Interchange Sort (Price Anomaly Validator)\n";
    vector<Bid> bidsC;
    loadBids("test_data/small.txt", bidsC);
    interchangeSort(bidsC);
    cout << "Sorted sample verification: " << (isSortedAscending(bidsC) ? "PASS [OK]" : "FAIL") << "\n\n";

    // --------------------------------------------------
    // Task D: Bubble Sort with Early Stop (Stabilization Detector)
    // --------------------------------------------------
    cout << ">>> [Task D] Bubble Sort with Early Stop (Stabilization Detector)\n";
    // Case 1: Already-sorted input
    vector<Bid> sortedStream = bidsA; // Already sorted from Task A
    bool isStable1 = bubbleSortEarlyStop(sortedStream);
    cout << "  Test 1 (Already-sorted stream): " << (isStable1 ? "STABLE" : "UNSTABLE")
         << " (Expected: STABLE) -> " << (isStable1 ? "PASS" : "FAIL") << "\n";

    // Case 2: Reversed input
    vector<Bid> reversedStream = bidsB; // Descending sorted from Task B is reversed of ascending
    bool isStable2 = bubbleSortEarlyStop(reversedStream);
    cout << "  Test 2 (Reversed stream):       " << (isStable2 ? "STABLE" : "UNSTABLE")
         << " (Expected: UNSTABLE) -> " << (!isStable2 ? "PASS" : "FAIL") << "\n\n";

    // --------------------------------------------------
    // Task E: Quick Sort (Full Historical Sorter)
    // --------------------------------------------------
    cout << ">>> [Task E] Quick Sort (Full Historical Sorter - 100,000 bids)\n";
    vector<Bid> bidsLarge;
    if (!loadBids("test_data/large.txt", bidsLarge)) {
        cerr << "Error: Could not load test_data/large.txt\n";
        return 1;
    }
    cout << "Loaded " << bidsLarge.size() << " records from test_data/large.txt.\n";
    cout << "Starting QuickSort with Median-of-Three pivot...\n";

    auto start = high_resolution_clock::now();
    quickSortWrapper(bidsLarge);
    auto end = high_resolution_clock::now();

    auto durationMs = duration_cast<milliseconds>(end - start).count();
    auto durationUs = duration_cast<microseconds>(end - start).count();

    cout << "QuickSort completed in: " << durationMs << " ms (" << durationUs << " us)\n";
    bool sortedOk = isSortedAscending(bidsLarge);
    cout << "Validation (100,000 records strictly sorted): " << (sortedOk ? "PASS [OK]" : "FAIL") << "\n";
    cout << "First 3 sorted records:\n";
    printBids(bidsLarge, 3);

    cout << "\n=====================================================\n";
    cout << "              All Test Modules Completed             \n";
    cout << "=====================================================\n";

    return 0;
}
