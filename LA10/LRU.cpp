/*
-------------------------------------------------------------------------------
ASSIGNMENT - 10 | Demand Paging with Page Replacement
-------------------------------------------------------------------------------
Name: Tuhin Mondal
Roll No: 22CS10087
-------------------------------------------------------------------------------
NOTE:

1) This code is a simulation of a demand paging system
2) Replacement algorithm used: LRU Approximation
3) Change input file name if required in FILE_NAME
4) Ouput varies slightly in each run due to randomization (in attempt 4)
5) Run with -DVERBOSE to see detailed output
-------------------------------------------------------------------------------
*/

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <queue>

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

inline void set_ref(unsigned short &x)      { x |= (1 << 14); }
inline void unset_ref(unsigned short &x)    { x &= ~(1 << 14); }
inline bool check_ref(unsigned short x)     { return (x & (1 << 14)) != 0; }

/*
FREE FRAME LIST
FRAME STRUCTURE DESCRIPTION:

- Each frame in the free-frame list is represented by a structure.
- Each frame has the following attributes:
    - frame: the frame number
    - last_pid: process ID of the last owner of the frame
    - last_page: page number of the last owner of the frame
*/

struct Frame {
    int frame;
    int last_pid;
    int last_page;

    Frame() : frame(-1), last_pid(-1), last_page(-1) {}
};

/*
PAGE TABLE ENTRY
DESCRIPTION:

- Entry (16 bits) at positions 0-15
    - Valid bit (1 bit) at position 15
    - Reference bit (1 bit) at position 14
    - Frame number (14 bits) at positions 0-13

- History (16 bits) at positions 0-15
- History is used for LRU approximation
*/

struct PAGE_TABLE_ENTRY {
    unsigned short entry;
    unsigned short history;
};

/*
PROCESS STRUCTURE
DESCRIPTION:    

- Each process has a page table with the following attributes:

    - size: number of pages in the process
    - search_indices: indices of pages to be searched
    - PT: array of page table entries
    - searches_done: number of searches done
    - is_active: flag indicating if the process is active
*/

struct Process {
    int size;
    int search_indices[MAX_SEARCHES];
    PAGE_TABLE_ENTRY PT[PAGES_PER_PROCESS];
    int searches_done;
    bool is_active;

    Process() : size(0), searches_done(0), is_active(false) {
        for (int i = 0; i < PAGES_PER_PROCESS; i++) {
            PT[i].entry = 0;
            PT[i].history = 0;
        }
    }
};

int n, m;
int NFF = 0;

/*
NECESSARY STRUCTURES:

- FFLIST: Free Frame List
- processes: Array of processes
- READY_Q: Queue of processes ready to be executed
- page_faults: Array to store page faults for each process
- page_accesses: Array to store page accesses for each process
- page_replacements: Array to store page replacements for each process
- attempts: 2D array to store attempts for each process
*/

Process* processes = NULL;
queue<Frame> FFLIST;
vector<Frame> temp; 
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

void _INIT_FFLIST_() {
    while (!FFLIST.empty()) FFLIST.pop();
    
    for (int i = 0; i < TOTAL_FRAMES; i++) {
        Frame free_frame;
        free_frame.frame = i;
        free_frame.last_pid = -1;
        free_frame.last_page = -1;
        FFLIST.push(free_frame);
    }
    NFF = TOTAL_FRAMES;
}

bool get_free_frame(Frame &f) {
    if (NFF > NFFMIN) {
        f = FFLIST.front();
        FFLIST.pop();
        NFF--;
        return true;
    }
    return false;
}

void update_history(Process* p) {
    for (int i = ESSENTIAL_PAGES; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->PT[i].entry)) 
        {
            unsigned short entry = p->PT[i].entry;
            bool flag = check_ref(entry);
            unsigned short ref = flag ? 1 : 0;

            unsigned short history = p->PT[i].history;
            p->PT[i].history = (history >> 1) | (ref << 15);
            unset_ref(p->PT[i].entry);
        }
    }
}


bool replace_LRU(Process* p, int requested_page, int process_number) {
    int victim = -1;
    unsigned short minHist = 0xFFFF;
    for (int j = ESSENTIAL_PAGES; j < PAGES_PER_PROCESS; j++) {
        if (check_valid(p->PT[j].entry)) {
            if (p->PT[j].history < minHist) {
                minHist = p->PT[j].history;
                victim = j;
            }
        }
    }
    if (victim == -1) {
        for (int j = ESSENTIAL_PAGES; j < PAGES_PER_PROCESS; j++) {
            if (check_valid(p->PT[j].entry)) {
                if (p->PT[j].history == minHist)
                {
                    minHist = p->PT[j].history;
                    victim = j;
                }
            }
        }
    }
    
    #ifdef VERBOSE
    cout << "\tFault on Page " << requested_page 
         << ": To replace Page " << victim 
         << " at Frame " << (p->PT[victim].entry & 0x3FFF)
         << " [history = " << minHist << "]" << endl;
    #endif
    
    // Copy queue to vector for searching
    temp.clear();
    while (!FFLIST.empty()) {
        temp.push_back(FFLIST.front());
        FFLIST.pop();
    }
    
    Frame FREE_FRAME;
    int FRAME_NO = -1;

    /* ATTEMPT 1:

    - We need to find a free frame f that was recently used by process_number.
    - Check if there is a free frame f whose last owner is process_number
    - Check if the page number stored in f is exactly the page_index.
    - This indicates that page p of process_number was replaced recently (reload the page without disk I/O)
    */
    for (int i = 0; i < (int)temp.size(); i++) {
        if (temp[i].last_pid == process_number && temp[i].last_page == requested_page) {
            FRAME_NO = i;
            attempts[process_number][0]++;
            #ifdef VERBOSE
            cout << "\t\tAttempt 1: ";
            cout << "Page found in free frame " << temp[i].frame << endl;
            #endif
            break;
        }
    }

    /* ATTEMPT 2:

    - If Attempt 1 fails, search for a free frame f that has no owner (pid == -1).
    - This frame has not been recently allocated to any process.
    - If such a frame is found, assign it to page p.
    */
    if (FRAME_NO == -1) {
        for (int i = temp.size() - 1; i >= 0; i--) {
            if (temp[i].last_pid == -1) {
                FRAME_NO = i;
                attempts[process_number][1]++;
                #ifdef VERBOSE
                cout << "\t\tAttempt 2: ";
                cout << "Free frame " << temp[i].frame;
                cout << " owned by no process found" << endl;
                #endif
                break;
            }
        }
    }

    /* ATTEMPT 3:

     - If Attempts 1 and 2 are unsuccessful
    - Look for any free frame f whose last owner was process_number, regardless of the page number in f.
    - This might indicate that the process previously used this frame for a different page.
    - If found, allocate f to page p.
    */
    if (FRAME_NO == -1) {
        for (int i = temp.size() - 1; i >= 0; i--) {
            if (temp[i].last_pid == process_number) {
                FRAME_NO = i;
                attempts[process_number][2]++;
                #ifdef VERBOSE
                cout << "\t\tAttempt 3: ";
                cout << "Own page " << temp[i].last_page; 
                cout << " found in free frame " << temp[i].frame << endl;
                #endif
                break;
            }
        }
    }

    /* ATTEMPT 4:

    - As a last resort, if no suitable frame is found in the above attempts.
    - It will simply select any random free frame from FFLIST and allocate it to page p.
    - This approach ensures that the simulation continues even if preferred frames are unavailable.
    */
    
    if (FRAME_NO == -1) {
        FRAME_NO = rand() % temp.size();
        attempts[process_number][3]++;
        #ifdef VERBOSE
        cout << "\t\tAttempt 4: ";
        cout << "Free frame " << temp[FRAME_NO].frame; 
        cout << " owned by Process " << temp[FRAME_NO].last_pid << " chosen" << endl;
        #endif
    }
    
    // Save the chosen free frame
    FREE_FRAME = temp[FRAME_NO];
    int FRAME_POS = p->PT[victim].entry & 0x3FFF;
    
    // Create a new free frame entry for the victim
    Frame freed;
    freed.frame = FRAME_POS;
    freed.last_pid = process_number;
    freed.last_page = victim;
    
    // Put chosen frame back in queue (replacing it with victim frame)
    temp[FRAME_NO] = freed;
    for (int i = 0; i < (int)temp.size(); i++) FFLIST.push(temp[i]);

    p->PT[requested_page].entry = (FREE_FRAME.frame & 0x3FFF);
    set_valid(p->PT[requested_page].entry);
    set_ref(p->PT[requested_page].entry);
    p->PT[requested_page].history = 0xFFFF;
    
    // Invalidate victim page
    unset_valid(p->PT[victim].entry);
    p->PT[victim].history = 0;
    
    page_replacements[process_number]++;
    return true;
}

void load_page(Process* p, int page_index, int process_number) {
    page_faults[process_number]++;
    
    if (NFF > NFFMIN) {
        Frame free_frame;
        if (get_free_frame(free_frame)) {
            #ifdef VERBOSE
            cout << "\tFault on Page " << page_index;
            cout << ": Free frame " << free_frame.frame;
            cout << " found" << endl;
            #endif
            free_frame.last_pid = process_number;
            free_frame.last_page = page_index;
            p->PT[page_index].entry = free_frame.frame;
            set_valid(p->PT[page_index].entry);
            set_ref(p->PT[page_index].entry);
            p->PT[page_index].history = 0xFFFF;
        } else {
            cerr << "[-] Can't allocate page" << endl;
            exit(EXIT_FAILURE);
        }
    } else {
        if (!replace_LRU(p, page_index, process_number)) {
            cerr << "[-] Page replacement error for process " << process_number << endl;
            exit(EXIT_FAILURE);
        }
    }
}

void REMOVE_PROCESS(int process_number) {
    Process* p = &processes[process_number];
    p->is_active = false;
    
    for (int i = 0; i < PAGES_PER_PROCESS; i++) {
        if (check_valid(p->PT[i].entry)) {
            int frame = p->PT[i].entry & 0x3FFF;
            unset_valid(p->PT[i].entry);
            
            // Add frame back to FFLIST with no owner
            Frame free_frame;
            free_frame.frame = frame;
            free_frame.last_pid = -1;
            free_frame.last_page = -1;
            FFLIST.push(free_frame);
            NFF++;
        }
    }
    // cout << "++++ NFF = " << NFF << endl;
}

bool search(int process_number) {
    Process *p = &processes[process_number];
    int key = p->search_indices[p->searches_done];
    int L = 0;
    int R = p->size - 1;
    
    while (L < R) {
        int M = L + ((R - L) >> 1);
        int page_index = ESSENTIAL_PAGES + M / 1024;
        
        if (!check_valid(p->PT[page_index].entry)) {
            load_page(p, page_index, process_number);
        } else {
            page_accesses[process_number]++;
            set_ref(p->PT[page_index].entry);
            
            if (key <= M) R = M;
            else L = M + 1;
        }
    }
    update_history(p);
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

int main() {
    srand(time(NULL));
    _INIT_FFLIST_();

    for (int i = 0; i < MAX_PROCESSES; i++) {
        page_faults[i] = 0;
        page_accesses[i] = 0;
        page_replacements[i] = 0;
        for (int j = 0; j < 4; j++) attempts[i][j] = 0;
    }

    ifstream file(FILE_NAME);
    if (!file.is_open()) {
        cerr << "Error opening input file: " << FILE_NAME << endl;
        exit(EXIT_FAILURE);
    }

    file >> n >> m;
    if (n > MAX_PROCESSES) { cerr << "[-] Number of processes exceeds limit\n"; exit(EXIT_FAILURE); }
    if (m > MAX_SEARCHES)  { cerr << "[-] Number of searches exceeds limit\n";  exit(EXIT_FAILURE); }
    processes = new Process[n];

    for (int i = 0; i < n; i++) {
        Process *p = &processes[i];
        p->is_active = true;
        p->searches_done = 0;

        file >> p->size;
        for (int j = 0; j < m; j++) file >> p->search_indices[j];
        for (int j = 0; j < ESSENTIAL_PAGES; j++)
        {
            Frame free_frame;
            if (get_free_frame(free_frame))
            {
                int fn = free_frame.frame;
                PAGE_TABLE_ENTRY &pte = p->PT[j];
                free_frame.last_pid = i;
                free_frame.last_page = j;
                pte.entry = fn;

                set_valid(pte.entry);
                set_ref(pte.entry);
                pte.history = 0xFFFF;
            }
            else
            {
                cerr << "[-] Not enough free frames during initialization" << endl;
                exit(EXIT_FAILURE);
            }
        }
        READY_Q.push(i);
    }
    file.close();
    cout << "+++ Simulation data read from file\n";

    run_simulation(); // -- Main simulation loop
    cout << "+++ Simulation completed\n";

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

    for (int i = 0; i < n; i++) {
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
        
        for (int j = 0; j < 4; j++)  cout << setw(indents[j]) << attempts[i][j] << ((j < 3) ? " + " : "  (");
        for (int j = 0; j < 4; j++)  cout << setw(5) << attempt_percent[j] << " %" << ((j < 3) ? " + " : ")\n");
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

    for (int i = 0; i < 4; i++) cout << setw(indents[i]) << attempt_cnt[i] << ((i < 3) ? " + " : "  (");
    for (int i = 0; i < 4; i++) cout << setw(5) << attempt_pcnt[i] << " %" << ((i < 3) ? " + " : ")\n");
    cout << string(125, '-') << endl;
    cout << endl;
    
    delete[] processes;
    return 0;
}