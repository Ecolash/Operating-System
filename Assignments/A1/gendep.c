#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define N_DEFAULT 50

void bsort(int *A, int n) {
   for (int j = n - 2; j >= 0; --j)
   for (int i = 0; i <= j; ++i)
      if (A[i] > A[i + 1]) {
            int t = A[i];
            A[i] = A[i + 1];
            A[i + 1] = t;
      }
}

int main(int argc, char *argv[]) {
    int n = (argc == 1) ? N_DEFAULT : atoi(argv[1]);
    srand((unsigned int)time(NULL));

    int *P = malloc(n * sizeof(int));
    int *R = malloc((n + 1) * sizeof(int));
    for (int i = 0; i < n; ++i) P[i] = i + 1;
    for (int i = n - 1; i > 0; --i) {
        int j = rand() % (i + 1);
        int t = P[i];
        P[i] = P[j];
        P[j] = t;
    }
    for (int i = 0; i < n; ++i) R[P[i]] = i;

    int **A = malloc(n * sizeof(int *));
    for (int i = 0; i < n; ++i) {
        A[i] = calloc(n, sizeof(int));
        for (int j = i + 1; j < n; ++j)
            A[i][j] = (rand() % 3 == 0) ? 1 : 0;
    }

    int *B = malloc(n * sizeof(int));
    FILE *fp = fopen("foodep.txt", "w");
    fprintf(fp, "%d\n", n);
    for (int i = 1; i <= n; ++i) {
        int t = R[i], k = 0;
        for (int j = 0; j < n; ++j)
            if (A[t][j]) B[k++] = P[j];
        bsort(B, k);
        fprintf(fp, "%d:", P[t]);
        for (int j = 0; j < k; ++j)
            fprintf(fp, " %d", B[j]);
        fprintf(fp, "\n");
    }
    fclose(fp);

    for (int i = 0; i < n; ++i) free(A[i]);
    free(A);
    free(B);
    free(P);
    free(R);
    return 0;
}
