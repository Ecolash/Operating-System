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
3) Change input file name if required in FILE_NAME
-------------------------------------------------------------
*/

#include <iostream>
#include <fstream>
#include <string>
#include <queue>
#include <iomanip>
#include <stdexcept>

using namespace std;

const char FILE_NAME[] = "search.txt";
const int indents[4] = {4, 4, 5, 2};

const int MAX_PROCESSES = 500;
const int MAX_SEARCHES = 100;
const int PAGES_PER_PROCESS = 2048;
const int TOTAL_FRAMES = 12288;
const int ESSENTIAL_PAGES = 10;
const int NFFMIN = 1000;

inline void set_valid(unsigned short &x)      {  x |= (1 << 15); }
inline void unset_valid(unsigned short &x)    {  x &= ~(1 << 15);  }
inline bool check_valid(unsigned short x)     { return (x & (1 << 15)) != 0; }

inline void set_reference(unsigned short &x)      { x |= (1 << 14); }
inline void unset_reference(unsigned short &x)    { x &= ~(1 << 14); }
inline bool check_reference(unsigned short x)     { return (x & (1 << 14)) != 0; }

struct Frame {
    int is_free;
    int pid;
    int page_number;
};

struct PAGE_ENTRY {
    unsigned short entry;
    unsigned short history;
};

struct Process {
    int size;
    int search_indices[MAX_SEARCHES];
    PAGE_ENTRY page_table[PAGES_PER_PROCESS];
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
int NFF;
Process* processes = NULL;
Frame FFLIST[TOTAL_FRAMES];
queue<int> READY_Q;

int page_faults[MAX_PROCESSES];
int page_accesses[MAX_PROCESSES];
int page_replacements[MAX_PROCESSES];
int attempts[MAX_PROCESSES][4];

double percentage(int part, int total) {
    if (total == 0) 
    {
        cerr << "[-] Division by 0" << endl;
        exit(EXIT_FAILURE);
    }
    double x = (double) part;
    double y = (double) total;
    return (x * 100.00) / y;
}

void update_page_history(Process* p) {
    for (int i = ESSENTIAL_PAGES; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->page_table[i].entry)) 
        {
            p->page_table[i].history >>= 1; // Right shift history
            if (check_reference(p->page_table[i].entry)) 
            {
                p->page_table[i].history |= (1 << 15);
                unset_reference(p->page_table[i].entry);
            }
        }
    }
}

void init_essential_pages(Process* p, int process_number) {
    for (int j = 0; j < ESSENTIAL_PAGES; j++) {
        int frame = TOTAL_FRAMES - NFF;
        if (frame < 0 || frame >= TOTAL_FRAMES) {
            cerr << "Frame index : " << frame << " out of range" << endl;
            exit(EXIT_FAILURE);
        }
        p->page_table[j].entry = frame;
        p->page_table[j].history = 0xFFFF;
        set_valid(p->page_table[j].entry);
        FFLIST[frame].pid = process_number;
        FFLIST[frame].page_number = j;
        FFLIST[frame].is_free = 0;
        NFF--;
    }
}

int replace_LRU(Process* p) {
    int min_history = 0xFFFF;
    int min_index = -1;
    for (int i = 10; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->page_table[i].entry)) 
        {
            int history = p->page_table[i].history;
            if (history < min_history) 
            {
                // cout << "Page " << i << " history: " << history << endl;
                min_history = history;
                min_index = i;
            }
        }
    }
    if (min_index == -1) {
        cerr << "[-] No valid page found for replacement" << endl;
        exit(EXIT_FAILURE);
    }
    return min_index;
}

void fetch_free_frame(Process* p, int page_index, int process_number) 
{
    /* ATTEMPT 1:

    - We need to find a free frame f that was recently used by process_number.
    - Check if there is a free frame f whose last owner is process_number 
    - Check if the page number stored in f is exactly the page_index. 
    - This indicates that page p of process_number was replaced recently (reload the page without disk I/O)
    */

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (FFLIST[i].is_free && FFLIST[i].pid == process_number && FFLIST[i].page_number == page_index) {
            #ifdef VERBOSE 
            cout << "\t\tAttempt 1: Page found in free frame " << i << endl;
            #endif
            FFLIST[i].is_free = 0;
            p->page_table[page_index].entry = i;
            set_valid(p->page_table[page_index].entry);
            attempts[process_number][0]++;
            return;
        }
    }

    /* ATTEMPT 2:

    - If Attempt 1 fails, search for a free frame f that has no owner (pid == -1).
    - This frame has not been recently allocated to any process.
    - If such a frame is found, assign it to page p.
    */

    for (int i = TOTAL_FRAMES - 1; i >= 0; i--) {
        if (FFLIST[i].is_free && FFLIST[i].pid == -1) {
            #ifdef VERBOSE 
                cout << "\t\tAttempt 2: Free frame " << i << " owned by no process found" << endl;
            #endif
            FFLIST[i].is_free = 0;
            FFLIST[i].pid = process_number;
            FFLIST[i].page_number = page_index;
            p->page_table[page_index].entry = i;
            set_valid(p->page_table[page_index].entry);
            attempts[process_number][1]++;
            return;
        }
    }

    /* ATTEMPT 3:

    - If Attempts 1 and 2 are unsuccessful
    - Look for any free frame f whose last owner was process_number, regardless of the page number in f.
    - This might indicate that the process previously used this frame for a different page.
    - If found, allocate f to page p.
    */

    for (int i = 0; i < TOTAL_FRAMES; i++) {
        if (FFLIST[i].is_free && FFLIST[i].pid == process_number) {
            #ifdef VERBOSE 
                cout << "\t\tAttempt 3: Own page " << FFLIST[i].page_number;
                cout << " found in free frame " << i << endl;
            #endif
            FFLIST[i].is_free = 0;
            FFLIST[i].pid = process_number;
            FFLIST[i].page_number = page_index;
            p->page_table[page_index].entry = i;
            set_valid(p->page_table[page_index].entry);
            attempts[process_number][2]++;
            return;
        }
    }

    /* ATTEMPT 4:

    - As a last resort, if no suitable frame is found in the above attempts.
    - It will simply select any random free frame from FFLIST and allocate it to page p.
    - This approach ensures that the simulation continues even if preferred frames are unavailable.
    */

    for (int i = TOTAL_FRAMES - 1; i >= 0; i--) {
        if (FFLIST[i].is_free) {
            #ifdef VERBOSE 
                cout << "\t\tAttempt 4: Free frame " << i;
                cout << " owned by Process " << FFLIST[i].pid;
                cout << " chosen" << endl;
            #endif
            FFLIST[i].is_free = 0;
            FFLIST[i].pid = process_number;
            FFLIST[i].page_number = page_index;
            p->page_table[page_index].entry = i;
            set_valid(p->page_table[page_index].entry);
            attempts[process_number][3]++;
            return;
        }
    }
}

void load_page(Process* p, int page_index, int process_number) {
    page_faults[process_number]++;
    
    if (NFF > NFFMIN) {
        #ifdef VERBOSE
            cout << "\tFault on page " << page_index;
            cout << ": Free frame " << TOTAL_FRAMES - NFF << " found" << endl;
        #endif
        int frame = TOTAL_FRAMES - NFF;
        if (frame < 0 || frame >= TOTAL_FRAMES)
        {
            cerr << "Frame index : " << frame << " out of range" << endl;
            exit(EXIT_FAILURE);
        }

        p->page_table[page_index].entry = frame;
        set_valid(p->page_table[page_index].entry);
        FFLIST[frame].pid = process_number;
        FFLIST[frame].page_number = page_index;
        FFLIST[frame].is_free = 0;
        NFF--;
    } else {
        page_replacements[process_number]++;
        int victim_page = replace_LRU(p);

        int frame = p->page_table[victim_page].entry;
        frame &= 0x3FFF; // Clear the valid bit
        if (frame < 0 || frame >= TOTAL_FRAMES)
        {
            cerr << "Frame index : " << frame << " out of range" << endl;
            exit(EXIT_FAILURE);
        }
        
        #ifdef VERBOSE
            int history = p->page_table[victim_page].history;
            cout << "\tFault on page " << page_index;
            cout << ": To replace Page " << victim_page << " at Frame " << frame;
            cout << " [history = " << history << "]" << endl;
        #endif
        
        fetch_free_frame(p, page_index, process_number);
        p->page_table[victim_page].entry = 0;
        p->page_table[victim_page].history = 0;
        FFLIST[frame].is_free = 1;
    }
}

void REMOVE_PROCESS(int process_number) {
    Process* p = &processes[process_number];
    p->is_active = false;
    
    for (int i = 0; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->page_table[i].entry)) {
            int frame = p->page_table[i].entry & 0x3FFF;
            if (frame < 0 || frame >= TOTAL_FRAMES)
                throw out_of_range("Frame index out of range");
            unset_valid(p->page_table[i].entry);
            
            FFLIST[frame].is_free = 1;
            FFLIST[frame].pid = -1;
            FFLIST[frame].page_number = -1;
            NFF++;
        }
    }
}

bool search(int process_number) {
    Process *p = &processes[process_number];
    int key = p->search_indices[p->searches_done];
    int L = 0;
    int R = p->size - 1;
    
    while (L < R) {
        int M = L + ((R - L) >> 1);
        int page_index = 10 + M / 1024;
        
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
            int searches_done = processes[i].searches_done;
            if (processes[i].is_active && searches_done < m) {
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
                cout << "+++ Process " << curr_process;
                cout << ": Search " << p->searches_done + 1 << endl;
            #endif
            
            if (search(curr_process)) {
                if (p->searches_done < m) READY_Q.push(curr_process);
                else REMOVE_PROCESS(curr_process);
            }
        }
    }
}


int main() 
{
    NFF = TOTAL_FRAMES;
    for (int i = 0; i < TOTAL_FRAMES; i++)
    {
        FFLIST[i].is_free = 1;
        FFLIST[i].pid = -1;
        FFLIST[i].page_number = -1;
    }
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        page_faults[i] = 0;
        page_accesses[i] = 0;
        page_replacements[i] = 0;

        attempts[i][0] = 0; // ATTEMPT TYPE 1
        attempts[i][1] = 0; // ATTEMPT TYPE 2
        attempts[i][2] = 0; // ATTEMPT TYPE 3
        attempts[i][3] = 0; // ATTEMPT TYPE 4
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

        int size;
        file >> size;
        p->size = size;

        for (int j = 0; j < m; j++) file >> p->search_indices[j];
        for (int j = 0; j < ESSENTIAL_PAGES; j++)
        {
            int frame = TOTAL_FRAMES - NFF;
            if (frame < 0 || frame >= TOTAL_FRAMES)
            {
                cerr << "Frame index out of range" << endl;
                exit(EXIT_FAILURE);
            }
            p->page_table[j].entry = frame;
            p->page_table[j].history = 0xFFFF;
            set_valid(p->page_table[j].entry);

            FFLIST[frame].pid = i;
            FFLIST[frame].page_number = j;
            FFLIST[frame].is_free = false;
            NFF--;
        }
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

    cout << fixed;
    cout << right;
    // cout << "\033[1m"; 
    cout << string(125, '-') << endl;
    cout << setw(5)  << "PID" << " |";
    cout << setw(10) << "Accesses" << " |";
    cout << setw(12)  << "Faults" << "      |";
    cout << setw(16) << "Replacements" << "   |";
    cout << setw(32) << "Attempts" << endl;
    cout << string(125, '-') << endl;
    // cout << "\033[0m"; 

    for (int i = 0; i < n; i++) 
    {
        access_cnt   += page_accesses[i];
        faults_cnt   += page_faults[i];
        replacements += page_replacements[i];
        for (int j = 0; j < 4; j++) attempt_cnt[j] += attempts[i][j];

        fault_percent = percentage(page_faults[i], page_accesses[i]);
        replacement_percent = percentage(page_replacements[i], page_accesses[i]);
        
        int total_cnt = attempts[i][0] + attempts[i][1] + attempts[i][2] + attempts[i][3];
        attempt_percent[0] = percentage(attempts[i][0], total_cnt);
        attempt_percent[1] = percentage(attempts[i][1], total_cnt);
        attempt_percent[2] = percentage(attempts[i][2], total_cnt);
        attempt_percent[3] = percentage(attempts[i][3], total_cnt);
        
        cout.precision(2);
        cout << fixed;
        cout << right;
        cout << setw(5) << i << " |" 
             << setw(10) << page_accesses[i] << " |"
             << setw(7) << page_faults[i] << " (" 
             << setw(5) << fault_percent << " %) |"
             << setw(7) << page_replacements[i] << " (" 
             << setw(5) << replacement_percent << " %)  |";
        
        for (int j = 0; j < 4; j++) cout << setw(indents[j]) << attempts[i][j] << ((j < 3) ? " + " : "  (");
        for (int j = 0; j < 4; j++) cout << setw(5) << attempt_percent[j] << " %" << ((j < 3) ? " + " : ")\n");
    }
    
    double fault_pcnt = percentage(faults_cnt, access_cnt);
    double replacement_pcnt = percentage(replacements, access_cnt);
    double attempt_pcnt[4];
    for (int j = 0; j < 4; j++)  attempt_pcnt[j] = percentage(attempt_cnt[j], replacements);
    
    cout << string(125, '-') << endl;
    cout << setw(5) << "Total" << " |";
    cout << setw(10) << access_cnt << " |";
    cout << setw(7)  << faults_cnt << " (";
    cout << setw(5)  << fault_pcnt << " %) |";
    cout << setw(7)  << replacements << " (";
    cout << setw(5)  << replacement_pcnt << " %)  |";

    for (int i = 0; i < 4; i++) cout << setw(indents[i]) << attempt_cnt[i] << ((i < 3) ? " + " : "  (");
    for (int i = 0; i < 4; i++) cout << setw(5) << attempt_pcnt[i] << " %" << ((i < 3) ? " + " : ")\n");
    cout << string(125, '-') << endl;
    cout << endl;    
    return 0;
}