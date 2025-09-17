# 🎓 Operating Systems Lab Assignments

This repository contains my solutions to a series of lab assignments for an Operating Systems course. The labs cover fundamental concepts of process and memory management, inter-process communication (IPC), and file systems, providing hands-on experience with low-level system programming.

***

## 📋 Topics Covered

* **LA1: Multi-process applications (fork, wait, exec)** — Process creation and management.
* **LA2: IPC using signals** — Inter-process communication with signals.
* **LA3: Scheduling Algo Simulation** — Analyze efficiency of CPU scheduling algorithms.
* **LA4: IPC using pipes and dup** — Implement IPC using pipes & file descriptor duplication.
* **LA5: IPC using shared memory** — Use shared memory for fast and efficient IPC.
* **LA6: IPC with shared memory + semaphores** — Combine SM with semaphores for thread-safe synchronization.
* **LA7: Multi-threading with pthreads** — Develop multi-threaded program using the `pthread`
* **LA8: Deadlock in multi-threaded apps** — Handle deadlock conditions in concurrent programs.
* **LA9: Demand paging (no replacement)** — Simulate a demand paging system without a page replacement algorithm.
* **LA10: Demand paging (with replacement)** — Extend the simulation to include a page replacement algorithm.
* **LA11: File system interface** — Work with the user-level interface of a file system.


***

## 🛠️ Languages & Tools

This project primarily uses the **C/C++ programming** languages and relies heavily on a **UNIX/Linux environment**. Key system calls and libraries used include:

* **Process Management:** `fork()`, `exec()`, `wait()`
* **IPC:** `pipe()`, `dup()`, `shmget()`, `shmat()`, `semget()`, `semop()`, `signal()`
* **Threading:** `pthread_create()`, `pthread_join()`

***

## 🚀 Clone Instructions

To get a copy of the repository, follow these steps.

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/your-username/your-repo-name.git](https://github.com/your-username/your-repo-name.git)
    ```
2.  **Navigate to the project directory:**
    ```bash
    cd your-repo-name
    ```
3.  **Explore the lab directories:** Each lab assignment is contained within its own folder (e.g., `LA1`).
    ```bash
    ls
    ```
4.  **To compile and run a specific assignment:**
    ```bash
    # Example for LA1
    cd LA1
    gcc -o gendep gendep.c
    ./gendep
    ```
    *Note: Some assignments, like those involving threads, will require linking the `-pthread` library during compilation.*
