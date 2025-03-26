#include <iostream>
#include <fstream>
#include <queue>

using namespace std;

const char FILENAME[] = "search.txt";

const int PAGES_PER_PROCESS = 2048;
const int TOTAL_FRAMES = 12288;
const int ESSENTIAL_PAGES = 10;

void set_valid(int &x)   { x |= (1 << 15); }
void clear_valid(int &x) { x &= ~(1 << 15); }
bool is_valid(int x)     { return (x & (1 << 15)) != 0; }

struct Process
{
    int size;
    int *search_indices;
    int *page_table;
    int searches_done;
    bool is_active;
};

int n, m;
int fault_cnt = 0;
int access_cnt = 0;
int min_active = 0;
int swaps = 0;

Process *processes = NULL;
queue<int> READY_Q;
queue<int> SWAP_OUT_Q;
queue<int> FREE_FRAME_Q;

void SWAP_OUT(int process_number)
{
    Process *p = &processes[process_number];
    p->is_active = false;
    for (int i = 0; i < PAGES_PER_PROCESS; i++)
    {
        if (is_valid(p->page_table[i]))
        {
            int frame = 0;
            for (int bit = 0; bit < 14; bit++) frame |= (p->page_table[i] & (1 << bit));
            // printf("Frame: %d\n", frame);
            clear_valid(p->page_table[i]);
            FREE_FRAME_Q.push(frame);
        }
    }

    if (p->searches_done < m)
    {
        int active = READY_Q.size();
        SWAP_OUT_Q.push(process_number);
        printf("+++ Swapping out process %3d ", process_number);
        printf(" [%3d active processes ]\n", active);
        if (active < min_active) min_active = active;
        swaps++;
    }
}

bool search(int pn)
{
    Process *p = &processes[pn];

    int L = 0;
    int R = p->size - 1;
    int key = p->search_indices[p->searches_done];
    while (L < R)
    {
        int M = L + (R - L) / 2;
        int pidx = 10 + (M >> 10);
        access_cnt++;

        bool valid = is_valid(p->page_table[pidx]);
        if (!valid)
        {
            fault_cnt++;
            if (!FREE_FRAME_Q.empty())
            {
                int frame = FREE_FRAME_Q.front();
                FREE_FRAME_Q.pop();
                p->page_table[pidx] = frame;
                set_valid(p->page_table[pidx]);
            }
            else 
            {
                SWAP_OUT(pn);
                return false;
            }
        }

        if (key <= M) R = M;
        else L = M + 1;
    }
    p->searches_done++;
    return true;
}

void SWAP_IN()
{
    if (FREE_FRAME_Q.size() < ESSENTIAL_PAGES) return;
    if (SWAP_OUT_Q.empty()) return;

    int process_number = SWAP_OUT_Q.front();
    SWAP_OUT_Q.pop();
    Process *p = &processes[process_number];

    for (int i = 0; i < ESSENTIAL_PAGES; i++)
    {
        int frame = FREE_FRAME_Q.front();
        FREE_FRAME_Q.pop();
        p->page_table[i] = frame;
        set_valid(p->page_table[i]);
    }

    int active_processes = READY_Q.size() + 1;
    printf("+++ Swapping in process %4d", process_number);
    printf("  [%3d active processes ]\n", active_processes);
    p->is_active = true;

    #ifdef VERBOSE
    cout << "\tSearch " << (p->searches_done + 1) << " by Process " << process_number << "\n";
    #endif

    if (search(process_number))
    {
        if (p->searches_done < m) READY_Q.push(process_number);
        else
        {
            SWAP_OUT(process_number);
            SWAP_IN();
        }
    }
}


int main()
{
    for (int i = 0; i < TOTAL_FRAMES; i++) FREE_FRAME_Q.push(i);

    ifstream file(FILENAME);
    if (!file.is_open()) cerr << "Error: File not found\n";

    file >> n >> m;
    processes = new Process[n];

    for (int i = 0; i < n; i++)
    {
        Process *p = &processes[i];
        p->is_active = true;
        p->searches_done = 0;
        p->page_table = new int[PAGES_PER_PROCESS](); 
        
        file >> p->size;
        p->search_indices = new int[m];

        for (int j = 0; j < m; j++) file >> p->search_indices[j];
        for (int j = 0; j < ESSENTIAL_PAGES; j++)
        {
            if (FREE_FRAME_Q.empty()) cerr << "Error: Not enough frames\n";
            int frame = FREE_FRAME_Q.front();
            FREE_FRAME_Q.pop();
            p->page_table[j] = frame;
            set_valid(p->page_table[j]);
        }

        READY_Q.push(i);
    }

    printf("+++ Simulation data read from file\n");
    printf("+++ Kernel data initialized\n");
    min_active = n;

    bool finished;
    do {
        int curr_process = READY_Q.front();
        READY_Q.pop();
        Process *p = &processes[curr_process];

        if (p->is_active && p->searches_done < m)
        {
            #ifdef VERBOSE
            cout << "\tSearch " << (p->searches_done + 1) << " by Process " << curr_process << "\n";
            #endif
            if (search(curr_process))
            {
                if (p->searches_done < m) READY_Q.push(curr_process);
                else
                {
                    SWAP_OUT(curr_process);
                    SWAP_IN();
                }
            }
        }

        finished = true;
        for (int i = 0; i < n; i++)
        {
            if (processes[i].searches_done < m)
            {
                finished = false;
                break;
            }
        }
    } while (!finished);
    printf("\n+++ Page access summary\n");
    printf("    Total number of page accesses  =  %-6d\n", access_cnt);
    printf("    Total number of page faults    =  %-6d\n", fault_cnt);
    printf("    Total number of swaps          =  %-6d\n", swaps);
    printf("    Degree of multiprogramming     =  %-6d\n", min_active);

    for (int i = 0; i < n; i++)
    {
        free(processes[i].page_table);
        free(processes[i].search_indices);
    }
    free(processes);
    return 0;
}