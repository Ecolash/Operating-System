/*
-------------------------------------------------------------
ASSIGNMENT - 10 | Demand Paging with Page Replacement
-------------------------------------------------------------
Name   : Tuhin Mondal
Roll No: 22CS10087
-------------------------------------------------------------
NOTE: 

1) This code is a simulation of a demand paging system
2) Replacement algorithm used: LRU Approximation 
3) The free frame implementation and allocation now mimic LRU1.cpp.
-------------------------------------------------------------
*/

#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <iomanip>
#include <stdexcept>
#include <cstdlib>
#include <ctime>

using namespace std;

const char FILE_NAME[] = "search.txt";
const int indents[4] = {4, 4, 5, 2};

const int MAX_PROCESSES = 500;
const int MAX_SEARCHES = 100;
const int PAGES_PER_PROCESS = 2048;
const int TOTAL_FRAMES = 12288;
const int ESSENTIAL_PAGES = 10;
const int NFFMIN = 1000;

/* Bit-manipulation functions for page table entry */
inline void set_valid(unsigned short &x)      {  x |= (1 << 15); }
inline void unset_valid(unsigned short &x)    {  x &= ~(1 << 15);  }
inline bool check_valid(unsigned short x)     { return (x & (1 << 15)) != 0; }

inline void set_reference(unsigned short &x)      { x |= (1 << 14); }
inline void unset_reference(unsigned short &x)    { x &= ~(1 << 14); }
inline bool check_reference(unsigned short x)     { return (x & (1 << 14)) != 0; }

/* Free Frame List using LRU1.cpp style */
struct FreeFrame {
    int frame;
    int last_owner;
    int last_page;
};

FreeFrame* fflist = nullptr; // Dynamic free frame list
int NFF = 0; // current number of free frames

/* Initialize the free frame list */
void init_fflist() {
    fflist = new FreeFrame[TOTAL_FRAMES];
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        fflist[i].frame = i;
        fflist[i].last_owner = -1;
        fflist[i].last_page = -1;
    }
    NFF = TOTAL_FRAMES;
}

/* Allocate a free frame normally if available (NFF > NFFMIN) */
bool allocate_free_frame(FreeFrame &f) {
    if (NFF > NFFMIN) {
        f = fflist[NFF - 1];
        NFF--;
        return true;
    }
    return false;
}

/* PAGE TABLE ENTRY (16-bit entry and history) */
struct PAGE_TABLE_ENTRY {
    unsigned short entry;
    unsigned short history;
};

struct Process {
    int size;
    int search_indices[MAX_SEARCHES];
    PAGE_TABLE_ENTRY page_table[PAGES_PER_PROCESS];
    int searches_done;
    bool is_active;

    Process() : size(0), searches_done(0), is_active(false) {
        for (int i = 0; i < PAGES_PER_PROCESS; i++) {
            page_table[i].entry = 0;
            page_table[i].history = 0;
        }
    }
};

int n, m;
Process* processes = nullptr;
queue<int> READY_Q;

/* Statistics arrays */
int page_faults[MAX_PROCESSES];
int page_accesses[MAX_PROCESSES];
int page_replacements[MAX_PROCESSES];
int attempts[MAX_PROCESSES][4]; // attempts[process][0..3]

double percentage(int part, int total) {
    if (total == 0) 
    {
        cerr << "[-] Division by 0" << endl;
        exit(EXIT_FAILURE);
    }
    return ((double)part * 100.00) / total;
}

/* Update page history – similar to LRU1.cpp */
void update_page_history(Process* p) {
    for (int i = ESSENTIAL_PAGES; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->page_table[i].entry)) {
            p->page_table[i].history = p->page_table[i].history >> 1;
            if (check_reference(p->page_table[i].entry)) {
                p->page_table[i].history |= (1 << 15);
                unset_reference(p->page_table[i].entry);
            }
        }
    }
}

/* Initialize essential pages using the free-frame list */
void init_essential_pages(Process* p, int process_number) {
    for (int j = 0; j < ESSENTIAL_PAGES; j++) {
        FreeFrame ff;
        if (allocate_free_frame(ff)) {
            ff.last_owner = process_number;
            ff.last_page = j;
            p->page_table[j].entry = ff.frame;
            set_valid(p->page_table[j].entry);
            p->page_table[j].history = 0xFFFF;
        } else {
            cerr << "Not enough free frames during essential page allocation for process " << process_number << endl;
            exit(EXIT_FAILURE);
        }
    }
}

/* Page replacement:
   - Selects a victim page (non-essential) having the minimum history.
   - Then obtains a free frame from fflist with four attempts.
   - The victim frame is freed (added back into fflist) and chosen free frame is
     assigned to the requested page.
*/
void page_replacement(Process* p, int requested_page, int process_number) {
    int victim = -1;
    unsigned short minHist = 0xFFFF;
    for (int i = ESSENTIAL_PAGES; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->page_table[i].entry)) {
            if (p->page_table[i].history < minHist) {
                minHist = p->page_table[i].history;
                victim = i;
            }
        }
    }
    if (victim == -1) {
        cerr << "[-] No valid page found for replacement" << endl;
        exit(EXIT_FAILURE);
    }
    #ifdef VERBOSE
        cout << "\tFault on Page " << requested_page 
             << ": To replace Page " << victim 
             << " at Frame " << (p->page_table[victim].entry & 0x3FFF)
             << " [history = " << minHist << "]" << endl;
    #endif

    int chosenIdx = -1;
    // Attempt 1: free frame with last_owner == process_number and last_page == requested_page.
    for (int i = 0; i < NFF; i++) {
        if (fflist[i].last_owner == process_number && fflist[i].last_page == requested_page) {
            chosenIdx = i;
            attempts[process_number][0]++;
            #ifdef VERBOSE
                cout << "\t\tAttempt 1: Page found in free frame " << fflist[chosenIdx].frame << endl;
            #endif
            break;
        }
    }
    
    // Attempt 2: free frame with no owner.
    if (chosenIdx == -1) {
        
        for (int i = 0; i < NFF; i++) {
            if (fflist[i].last_owner == -1) {
                chosenIdx = i;
                attempts[process_number][1]++;
                #ifdef VERBOSE
                    cout << "\t\tAttempt 2: Free frame " << fflist[chosenIdx].frame 
                         << " owned by no process found" << endl;
                #endif
                break;
            }
        }
    }

    // Attempt 3: free frame with last_owner == process_number.
    if (chosenIdx == -1) {
        for (int i = 0; i < NFF; i++) {
            if (fflist[i].last_owner == process_number) {
                chosenIdx = i;
                attempts[process_number][2]++;
                #ifdef VERBOSE
                    cout << "\t\tAttempt 3: Own page " << fflist[chosenIdx].last_page 
                         << " found in free frame " << fflist[chosenIdx].frame << endl;
                #endif
                break;
            }
        }
    }

    // Attempt 4: choose any free frame randomly.
    if (chosenIdx == -1) {
        chosenIdx = rand() % NFF;
        attempts[process_number][3]++;
        #ifdef VERBOSE
            cout << "\t\tAttempt 4: Free frame " << fflist[chosenIdx].frame 
                 << " owned by Process " << fflist[chosenIdx].last_owner << " chosen" << endl;
        #endif
    }
    // Save the chosen free frame and remove it from the free frame list
    FreeFrame chosenFrame = fflist[chosenIdx];

    // Free the victim page’s frame and add it into the free-frame list.
    int victimFrame = p->page_table[victim].entry & 0x3FFF;
    FreeFrame freed;
    freed.frame = victimFrame;
    freed.last_owner = process_number;
    freed.last_page = victim;
    fflist[chosenIdx] = freed;
    
    // Invalidate the victim page.
    unset_valid(p->page_table[victim].entry);
    p->page_table[victim].history = 0;
    page_replacements[process_number]++;

    // Assign chosen free frame to the requested (faulting) page.
    p->page_table[requested_page].entry = chosenFrame.frame;
    set_valid(p->page_table[requested_page].entry);
    p->page_table[requested_page].history = 0xFFFF;
}

/* Load a page into memory using the free-frame list mechanism */
void load_page(Process* p, int page_index, int process_number) {
    page_faults[process_number]++;
    FreeFrame ff;
    if (allocate_free_frame(ff)) {
#ifdef VERBOSE
        cout << "\tFault on page " << page_index << ": Free frame " << ff.frame << " allocated" << endl;
#endif
        p->page_table[page_index].entry = ff.frame;
        set_valid(p->page_table[page_index].entry);
        p->page_table[page_index].history = 0xFFFF;
    } else {
        page_replacement(p, page_index, process_number);
    }
}

/* Remove a process and release its frames back to the free frame list */
void REMOVE_PROCESS(int process_number) {
    Process* p = &processes[process_number];
    p->is_active = false;
    
    for (int i = 0; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->page_table[i].entry)) {
            int frame = p->page_table[i].entry & 0x3FFF;
            unset_valid(p->page_table[i].entry);
            // Add the frame back into the free frame list
            fflist[NFF].frame = frame;
            fflist[NFF].last_owner = -1;
            fflist[NFF].last_page = -1;
            NFF++;
        }
    }
}

/* Search performs a binary search simulation with demand paging */
bool search(int process_number) {
    Process *p = &processes[process_number];
    int key = p->search_indices[p->searches_done];
    int L = 0;
    int R = p->size - 1;
    
    while (L < R) {
        int M = L + ((R - L) >> 1);
        int page_index = ESSENTIAL_PAGES + M / 1024;
        
        if (!check_valid(p->page_table[page_index].entry)) {
            load_page(p, page_index, process_number);
            p->page_table[page_index].history = 0xFFFF;
        }
        
        page_accesses[process_number]++;
        set_reference(p->page_table[page_index].entry);
        
        if (key <= M) R = M;
        else L = M + 1;
    }
    update_page_history(p);
    p->searches_done++;
    return true;
}

void run_simulation() {
    while (true) {
        bool finished = true;
        for (int i = 0; i < n; i++) {
            if (processes[i].is_active && processes[i].searches_done < m) {
                finished = false;
                break;
            }
        }
        if (finished) return;
        
        int curr_process = READY_Q.front();
        Process* p = &processes[curr_process];
        READY_Q.pop();
        
        if (p->is_active && p->searches_done < m) {
#ifdef VERBOSE
            cout << "+++ Process " << curr_process << ": Search " << p->searches_done + 1 << endl;
#endif            
            if (search(curr_process)) {
                if (p->searches_done < m)
                    READY_Q.push(curr_process);
                else
                    REMOVE_PROCESS(curr_process);
            }
        }
    }
}

int main() 
{
    srand(time(NULL));
    init_fflist();

    // Initialize statistics arrays.
    for (int i = 0; i < MAX_PROCESSES; i++) {
        page_faults[i] = 0;
        page_accesses[i] = 0;
        page_replacements[i] = 0;
        for (int j = 0; j < 4; j++) {
            attempts[i][j] = 0;
        }
    }

    ifstream file(FILE_NAME);
    if (!file.is_open())
    {
        cerr << "Error opening input file: " << FILE_NAME << endl;
        exit(EXIT_FAILURE);
    }

    file >> n >> m;
    if (n > MAX_PROCESSES) { cerr << "[-] Number of processes exceeds limit\n"; exit(EXIT_FAILURE); }
    if (m > MAX_SEARCHES)  { cerr << "[-] Number of searches exceeds limit\n";  exit(EXIT_FAILURE); }
    processes = new Process[n];

    for (int i = 0; i < n; i++)
    {
        Process *p = &processes[i];
        p->is_active = true;
        p->searches_done = 0;

        file >> p->size;
        for (int j = 0; j < m; j++)
            file >> p->search_indices[j];

        // Allocate essential pages using the modified free frame list.
        init_essential_pages(p, i);
        READY_Q.push(i);
    }
    file.close();
    cout << "+++ Simulation data read from file\n";

    run_simulation();

    cout << "+++ Page access summary\n";
    int access_cnt = 0;
    int faults_cnt = 0;
    int replacements = 0;
    int attempt_cnt[4] = {0};
    double attempt_percent[4];
    double fault_percent = 0;
    double replacement_percent = 0;

    cout << fixed << right;
    cout << string(125, '-') << endl;
    cout << setw(5)  << "PID" << " |"
         << setw(10) << "Accesses" << " |"
         << setw(12)  << "Faults" << "      |"
         << setw(16) << "Replacements" << "   |"
         << setw(32) << "Attempts" << endl;
    cout << string(125, '-') << endl;

    for (int i = 0; i < n; i++) 
    {
        access_cnt   += page_accesses[i];
        faults_cnt   += page_faults[i];
        replacements += page_replacements[i];
        for (int j = 0; j < 4; j++) { attempt_cnt[j] += attempts[i][j]; }

        fault_percent = percentage(page_faults[i], page_accesses[i]);
        replacement_percent = percentage(page_replacements[i], page_accesses[i]);
        
        int total_attempts = attempts[i][0] + attempts[i][1] + attempts[i][2] + attempts[i][3];
        attempt_percent[0] = percentage(attempts[i][0], total_attempts);
        attempt_percent[1] = percentage(attempts[i][1], total_attempts);
        attempt_percent[2] = percentage(attempts[i][2], total_attempts);
        attempt_percent[3] = percentage(attempts[i][3], total_attempts);

        cout.precision(2);
        cout << right;
        cout << setw(5) << i << " |" 
             << setw(10) << page_accesses[i] << " |"
             << setw(7) << page_faults[i] << " (" 
             << setw(5) << fault_percent << " %) |"
             << setw(7) << page_replacements[i] << " (" 
             << setw(5) << replacement_percent << " %)  |";
        
        for (int j = 0; j < 4; j++) 
            cout << setw(indents[j]) << attempts[i][j] << ((j < 3) ? " + " : "  (");
        for (int j = 0; j < 4; j++) 
            cout << setw(5) << attempt_percent[j] << " %" << ((j < 3) ? " + " : ")\n");
    }
    
    double fault_pcnt = percentage(faults_cnt, access_cnt);
    double replacement_pcnt = percentage(replacements, access_cnt);
    double attempt_pcnt[4];
    for (int j = 0; j < 4; j++)  attempt_pcnt[j] = percentage(attempt_cnt[j], replacements);
    
    cout << string(125, '-') << endl;
    cout << setw(5) << "Total" << " |"
         << setw(10) << access_cnt << " |"
         << setw(7)  << faults_cnt << " ("
         << setw(5)  << fault_pcnt << " %) |"
         << setw(7)  << replacements << " ("
         << setw(5)  << replacement_pcnt << " %)  |";

    for (int i = 0; i < 4; i++) 
        cout << setw(indents[i]) << attempt_cnt[i] << ((i < 3) ? " + " : "  (");
    for (int i = 0; i < 4; i++) 
        cout << setw(5) << attempt_pcnt[i] << " %" << ((i < 3) ? " + " : ")\n");
    cout << string(125, '-') << endl;
    cout << endl;
    
    // Cleanup allocated memory.
    delete [] processes;
    delete [] fflist;
    
    return 0;
}
