#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void genschedule(int n)
{
    int i, j, b, IOmin = 50, CPUmin, CPUmax, IOmod = 151, CPUmod;
    int A = 0;
    FILE *fp;

    fp = (FILE *)fopen("proc.txt", "w");
    fprintf(fp, "%d\n", n);

    for (i = 1; i <= n; ++i)
    {
        fprintf(fp, "%5d %8d", i, A);
        if (rand() % 10)
        { /* IO-bound job */
            b = 4 + rand() % 7;
            CPUmin = 1;
            CPUmax = 15;
        }
        else
        { /* CPU-bound job */
            b = 3 + rand() % 5;
            CPUmin = 100;
            CPUmax = 300;
        }
        CPUmod = CPUmax - CPUmin + 1;
        for (j = 1; j < b; ++j)
            fprintf(fp, " %4d %4d", CPUmin + rand() % CPUmod, IOmin + rand() % IOmod);
        fprintf(fp, " %4d  -1\n", CPUmin + rand() % CPUmod);
        A += rand() % 500;
    }

    fclose(fp);
}

int main(int argc, char *argv[])
{
    int n;

    srand((unsigned int)time(NULL));
    n = (argc == 1) ? 100 : atoi(argv[1]);
    genschedule(n);
    exit(0);
}