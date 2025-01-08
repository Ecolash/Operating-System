#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

/*
DFS of a tree using fork() system call
Input:

10
0 1
0 2
0 3
1 4
1 5
2 6
2 7
4 8
6 9
*/

#define MAX_NODES 100

int tree[MAX_NODES][MAX_NODES];
int visited[MAX_NODES];
int markers[MAX_NODES];
int cnt[MAX_NODES];

void dfs_fork(int node, int depth, int last, int* markers) {
    for (int i = 0; i < depth - 1; i++) 
    {
        if (markers[i]) printf(" │  ");
        else printf("    ");
    }
    if (depth > 0) printf(last? " └─>" : " ├─> ");
    printf("Node %d [%d] \n", node, getpid());

    if (depth > 0) markers[depth - 1] = !last;
    visited[node] = 1;

    for (int i = 0; i < cnt[node]; i++) 
    {
        int x = tree[node][i];
        if (!visited[x]) {
            pid_t pid = fork();
            switch (pid) {
                case 0: dfs_fork(x, depth + 1, i == cnt[node] - 1, markers); exit(0);
                case -1: perror("Fork failed"); exit(1);
                default: wait(NULL);
            }
        }
    }
}

int main(int argc, char *argv[]) {
    int n, m, start;
    if (argc != 2) { printf("Usage: %s <start_node>\n", argv[0]); exit(1); }
    memset(visited, 0, sizeof(visited));
    memset(markers, 0, sizeof(markers));
    memset(cnt, 0, sizeof(cnt));

    start = atoi(argv[1]);
    printf("Enter the number of nodes (n): "); scanf("%d", &n);
    printf("Enter the %d edges of tree (u v):\n", n - 1);
    for (int i = 0; i < n - 1; i++) {
        int u, v; scanf("%d %d", &u, &v);
        tree[u][cnt[u]++] = v;
    }

    if (start < 0 || start >= n) { printf("Invalid start node\n"); exit(1); }

    printf("Starting DFS from node %d...\n", start);
    dfs_fork(start, 0, 1, markers);
    printf("DFS traversal completed.\n");
    return 0;
}
