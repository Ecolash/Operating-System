
#include <iostream>
#include <fstream>
#include <deque>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <ctime>
#include <bits/stdc++.h>

using namespace std;

/* Constants */
#define PAGE_SIZE         4096         // bytes
#define INT_SIZE          4            // bytes
#define INT_PER_PAGE      (PAGE_SIZE/INT_SIZE)  // 1024 integers per page

#define OS_MEMORY_MB      16
#define TOTAL_MEMORY_MB   64
#define USER_MEMORY_MB    (TOTAL_MEMORY_MB - OS_MEMORY_MB)
// Total user frames: 48MB / 4KB = 12288 frames
#define TOTAL_USER_FRAMES (USER_MEMORY_MB * 1024 * 1024 / PAGE_SIZE)

#define PT_ENTRIES        2048         // page table size per process
#define ESSENTIAL_PAGES   10           // pages 0-9 are essential; pages 10..2047 are for array A

// In our 16-bit page-table entry, we reserve:
// Bit15: valid, Bit14: reference; Bits 0-13: frame number.
#define VALID_MASK        0x8000       // 1000 0000 0000 0000
#define REF_MASK          0x4000       // 0100 0000 0000 0000
#define FRAME_MASK        0x3FFF       // lower 14 bits

// For history, we use a full 16-bit counter.
#define HISTORY_INIT      0xFFFF

// For global free-frame list threshold
#define NFFMIN            1000

/* Page Table Entry */
struct PTE {
    unsigned short entry;   // 16-bit: valid, reference, frame number
    unsigned short history; // 16-bit history counter
};

/* Process structure */
struct Process {
    int id;             // process id
    int s;              // size of array A (number of integers)
    int current_search; // next search to perform (0-indexed)
    int key;            // search key (not directly used here)
    PTE pt[PT_ENTRIES]; // page table (entries for 2048 pages)
    int* search_keys;   // array of m search keys (each is an index in A)
    int swapped;        // 0: active, 1: (not used here), 2: terminated
    int total_page_accesses;
    int total_page_faults;
    int total_page_replacements;
    int attempt1;
    int attempt2;
    int attempt3;
    int attempt4;
    // (Local search bounds are reinitialized each time)
    
    Process() : id(0), s(0), current_search(0), key(0), search_keys(nullptr), swapped(0), 
                total_page_accesses(0), total_page_faults(0), total_page_replacements(0),
                attempt1(0), attempt2(0), attempt3(0), attempt4(0) {
        // Initialize page table to all zeros.
        memset(pt, 0, sizeof(pt));
    }
};

int n, m;                // number of processes and searches per process
Process* proc_arr = nullptr;  // array of processes

// Global free-frame list (FFLIST) entry structure.
struct FreeFrame {
    int frame;      // frame number
    int last_owner; // PID of last owner (-1 if none)
    int last_page;  // last owner’s page number (-1 if none)
};

FreeFrame* fflist = nullptr; // array of FreeFrame of size TOTAL_USER_FRAMES
int NFF = 0;                 // number of free frames currently

// For round-robin scheduling, we use a deque of process IDs.
queue<int> readyQ;

/* Initialize the free-frame list */
void init_fflist() {
    fflist = new FreeFrame[TOTAL_USER_FRAMES];
    for (int i = 0; i < TOTAL_USER_FRAMES; i++) {
        fflist[i].frame = i;
        fflist[i].last_owner = -1;
        fflist[i].last_page = -1;
    }
    NFF = TOTAL_USER_FRAMES;
}

/* For a normal frame allocation (when NFF > NFFMIN),
   remove and return the first free frame.
   Returns true and assigns f if successful.
*/
bool allocate_free_frame(FreeFrame &f) {
    if (NFF > NFFMIN) {
        f = fflist[NFF-1];
        NFF--;
        return true;
    }
    return false;
}

/* Local page replacement for process p when a page fault occurs on requested_page.
   This function is called when NFF == NFFMIN.
   It selects a victim page (non-essential) from process p's page table (with minimum history)
   and then, using attempts 1-4, selects a free frame from the FFLIST.
   It then frees the victim page (adding its frame info to FFLIST) and assigns the chosen free frame
   to the requested_page.
   Returns true if successful.
*/
bool page_replacement(Process* p, int requested_page) {
    // Select victim page among non-essential pages (>= ESSENTIAL_PAGES)
    int victim = -1;
    unsigned short minHist = USHRT_MAX;
    for (int j = ESSENTIAL_PAGES; j < PT_ENTRIES; j++) {
        if (p->pt[j].entry & VALID_MASK) {
            if (p->pt[j].history < minHist) {
                minHist = p->pt[j].history;
                victim = j;
            }
        }
    }
    if (victim == -1) {
        // No victim found (should not happen)
        return false;
    }
    int pid = p->id;
#ifdef VERBOSE
    printf("    Fault on Page  %d: To replace Page %d at Frame %d [history = %d]\n", requested_page, victim, p->pt[victim].entry & FRAME_MASK, minHist);
#endif
    // Now try the four attempts:
    int chosenIdx = -1;
    // Attempt 1: free frame with last_owner == pid and last_page == requested_page.
    for (int i = 0; i < NFF; i++) {
        if (fflist[i].last_owner == pid && fflist[i].last_page == requested_page){
            chosenIdx = i;
            break;
        }
    }
    if (chosenIdx != -1) {
        p->attempt1++;
#ifdef VERBOSE
        printf("        Attempt 1: Page found in free frame %d\n", fflist[chosenIdx].frame);
#endif
    }
    else {
        // Attempt 2: free frame with no owner (last_owner == -1)
        for (int i = 0; i < NFF; i++) {
            if (fflist[i].last_owner == -1){
                chosenIdx = i;
                break;
            }
        }
        if (chosenIdx != -1) { 
            p->attempt2++;
#ifdef VERBOSE
            printf("        Attempt 2: Free frame %d owned by no process found\n", fflist[chosenIdx].frame);
#endif
        }
        else {
            // Attempt 3: free frame with last_owner == pid (any page different from requested_page)
            for (int i = 0; i < NFF; i++) {
                if (fflist[i].last_owner == pid){
                    chosenIdx = i;
                    break;
                }
            }
            if (chosenIdx != -1) {
                p->attempt3++;
#ifdef VERBOSE
                printf("        Attempt 3: Own page %d found in free frame %d\n", fflist[chosenIdx].last_page, fflist[chosenIdx].frame);
#endif
            }
            else {
                // Attempt 4: pick the first available (or random) free frame.
                chosenIdx = rand() % NFF;
                p->attempt4++;
#ifdef VERBOSE
                printf("        Attempt 4: Free frame %d owned by Process %d chosen\n", fflist[chosenIdx].frame, fflist[chosenIdx].last_owner);
#endif
            }
        }
    }
    // Remove the chosen free frame from FFLIST.
    FreeFrame newFrame = fflist[chosenIdx];
    // Now, free the victim page's frame from process p.
    int victimFrame = p->pt[victim].entry & FRAME_MASK;
    // Add victimFrame to FFLIST with last owner = pid and last_page = victim.
    FreeFrame freed;
    freed.frame = victimFrame;
    freed.last_owner = pid;
    freed.last_page = victim;
    // Append at end.
    fflist[chosenIdx] = freed;
    
    // Update page table: replace victim page with requested page.
    // (Set new frame, valid, reference, and history = 0xffff)
    p->pt[requested_page].entry = VALID_MASK | REF_MASK | (newFrame.frame & FRAME_MASK);
    p->pt[requested_page].history = HISTORY_INIT;
    // Invalidate victim page.
    p->pt[victim].entry = 0;
    p->pt[victim].history = 0;
    
    p->total_page_replacements++;
    return true;
}

/* After each binary search (time quantum), update history for each valid page.
   For each valid page, do:
*/
void update_page_history(Process* p) {
    for (int i = 0; i < PT_ENTRIES; i++) {
        if (p->pt[i].entry & VALID_MASK) {
            unsigned short ref = (p->pt[i].entry & REF_MASK) ? 1 : 0;
            p->pt[i].history = (p->pt[i].history >> 1) | (ref << 15);
            // Clear reference bit.
            p->pt[i].entry &= ~REF_MASK;
        }
    }
}

/* Main simulation function */
int main() {
    srand(time(NULL));
    
    // Initialize FFLIST.
    init_fflist();

    FILE* fp = fopen("search.txt", "r");
    if (fp == NULL) {
        perror("fopen search.txt");
        exit(1);
    }
    
    // Read n and m
    if (fscanf(fp, "%d %d", &n, &m) != 2) {
        cerr << "Error reading n and m" << endl;
        exit(1);
    }
    
    // Allocate process array
    proc_arr = new Process[n];
    
    // Each process, when loaded, gets its essential pages from the free-frame queue.
    for (int i = 0; i < n; i++) {
        proc_arr[i].id = i;
        proc_arr[i].current_search = 0;
        proc_arr[i].swapped = 0;
        int s;
        if (fscanf(fp, "%d", &s) != 1) {
            cerr << "Error reading size for process " << i << endl;
            exit(1);
        }
        proc_arr[i].s = s;
        proc_arr[i].search_keys = new int[m];
        for (int j = 0; j < m; j++) {
            int key;
            if (fscanf(fp, "%d", &key) != 1) {
                cerr << "Error reading search key for process " << i << ", search " << j << endl;
                exit(1);
            }
            proc_arr[i].search_keys[j] = key;
        }
        // Initialize page table to all zeros and load essential pages (pages 0 to 9)
        for (int j = 0; j < ESSENTIAL_PAGES; j++) {
            FreeFrame ff;
            if(allocate_free_frame(ff)){
                ff.last_owner = i;
                ff.last_page = j;
            }
            else {
                cerr << "Not enough free frames during initialization for process " << i << endl;
                exit(1);
            }
            proc_arr[i].pt[j].entry = VALID_MASK | REF_MASK | (ff.frame & FRAME_MASK);
            proc_arr[i].pt[j].history = HISTORY_INIT; // essential pages always have history = 0xffff.
        }
    }
    fclose(fp);
    
    // Initialize ready queue (round-robin scheduling) with all process IDs.
    for (int i = 0; i < n; i++) {
        readyQ.push(i);
    }
    
    int terminated = 0;
    
    // Main simulation loop: while not all processes terminated.
    while (terminated < n) {
        int pid = readyQ.front();
        readyQ.pop();
        Process* p = &proc_arr[pid];
        
        // Restart binary search bounds for this time quantum.
        int L = 0;
        int R = p->s - 1;
        int key = p->search_keys[p->current_search];
#ifdef VERBOSE
        printf("+++ Process %d: Search %d\n", p->id, p->current_search + 1);
#endif
        while (L < R) {
            int M = (L + R) / 2;
            // Compute logical page index for array A access.
            // Array A starts at page number ESSENTIAL_PAGES.
            int page_index = ESSENTIAL_PAGES + (M / INT_PER_PAGE);
            // Check if page is resident.
            if (p->pt[page_index].entry & VALID_MASK) {
                p->total_page_accesses++;
                // Page is resident: set reference bit.
                p->pt[page_index].entry |= REF_MASK;
                // Binary search step:
                if (key <= M)
                    R = M;
                else
                    L = M + 1;
            } else {
                // Page fault.
                p->total_page_faults++;
                // If free frames available above threshold, allocate normally.
                if (NFF > NFFMIN) {
                    FreeFrame ff;
                    if (allocate_free_frame(ff)) {
                        ff.last_owner = p->id;
                        ff.last_page = page_index;
                        p->pt[page_index].entry = VALID_MASK | REF_MASK | (ff.frame & FRAME_MASK);
                        p->pt[page_index].history = HISTORY_INIT;
#ifdef VERBOSE
                    printf("    Fault on Page  %d: Free frame %d found\n", page_index, ff.frame);
#endif
                    } else {
                        cerr << "Allocation error in normal path" << endl;
                        exit(1);
                    }
                } else {
                    // NFF == NFFMIN: perform page replacement.
                    if (!page_replacement(p, page_index)) {
                        cerr << "Page replacement error for process " << p->id << endl;
                        exit(1);
                    }
                }
                // After handling fault, do not change L,R; reattempt the same iteration.
            }
        } // end binary search loop
        
        // At the end of the binary search, update the page histories.
        update_page_history(p);
        
        // The search is complete for this time quantum.
        p->current_search++;
        if (p->current_search == m) {
            // Process terminates.
            p->swapped = 2;
            terminated++;
            // Release all frames held by this process into FFLIST with no owner.
            for (int j = 0; j < PT_ENTRIES; j++) {
                if (p->pt[j].entry & VALID_MASK) {
                    int frame_num = p->pt[j].entry & FRAME_MASK;
                    // Reset ownership info.
                    FreeFrame ff;
                    ff.frame = frame_num;
                    ff.last_owner = -1;
                    ff.last_page = -1;
                    fflist[NFF] = ff;
                    NFF++;
                    p->pt[j].entry = 0;
                    p->pt[j].history = 0;
                }
            }
        } else {
            // Process still has searches left; re-enqueue it.
            readyQ.push(pid);
        }
    } // end simulation loop

    // Print summary statistics (non-verbose output)
    cout << "+++ Page access summary" << endl;
    printf("%7s%13s%14s%21s%35s\n", "PID", "Accesses", "Faults", "Replacements", "Attempts");
    int total_page_accesses = 0;
    int total_page_faults = 0;
    int total_page_replacements = 0;
    int total_attempt1 = 0;
    int total_attempt2 = 0;
    int total_attempt3 = 0;
    int total_attempt4 = 0;
    char c = '(';
    for(int i = 0; i < n; i++) {
        Process* p = &proc_arr[i];
        double fault_rate = (double)p->total_page_faults / p->total_page_accesses * 100.0;
        double replacement_rate = (double)p->total_page_replacements / p->total_page_accesses * 100.0;
        int total_attempts = p->attempt1 + p->attempt2 + p->attempt3 + p->attempt4;
        double attempt1_rate = (double)p->attempt1 / total_attempts * 100.0;
        double attempt2_rate = (double)p->attempt2 / total_attempts * 100.0;
        double attempt3_rate = (double)p->attempt3 / total_attempts * 100.0;
        double attempt4_rate = (double)p->attempt4 / total_attempts * 100.0;
        total_page_accesses += p->total_page_accesses;
        total_page_faults += p->total_page_faults;
        total_page_replacements += p->total_page_replacements;
        total_attempt1 += p->attempt1;
        total_attempt2 += p->attempt2;
        total_attempt3 += p->attempt3;
        total_attempt4 += p->attempt4;
        // Print process statistics
        printf("%5d%13d%9d%4c%4.2f%%)%7d%4c%4.2f%%)      %4d +%4d +%4d +%4d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n", p->id, p->total_page_accesses, p->total_page_faults, c, fault_rate,
               p->total_page_replacements, c, replacement_rate, p->attempt1, p->attempt2, p->attempt3, p->attempt4, attempt1_rate, attempt2_rate, attempt3_rate, attempt4_rate);
    }
    printf("\n%7s%13d%9d%4c%4.2f%%)%7d%4c%4.2f%%)      %4d +%5d +%6d +%4d  (%5.2f%% + %5.2f%% + %5.2f%% + %5.2f%%)\n", "Total", 
           total_page_accesses, total_page_faults, c, (double)total_page_faults / total_page_accesses * 100.0,
           total_page_replacements, c, (double)total_page_replacements / total_page_accesses * 100.0,
           total_attempt1, total_attempt2, total_attempt3, total_attempt4,
           (double)total_attempt1 / (total_attempt1 + total_attempt2 + total_attempt3 + total_attempt4) * 100.0,
           (double)total_attempt2 / (total_attempt1 + total_attempt2 + total_attempt3 + total_attempt4) * 100.0,
           (double)total_attempt3 / (total_attempt1 + total_attempt2 + total_attempt3 + total_attempt4) * 100.0,
           (double)total_attempt4 / (total_attempt1 + total_attempt2 + total_attempt3 + total_attempt4) * 100.0);
    // Free allocated memory.
    for (int i = 0; i < n; i++) {
        delete [] proc_arr[i].search_keys;
    }

    delete [] proc_arr;
    delete [] fflist;
    
    return 0;
}
