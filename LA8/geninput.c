#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main ( int argc, char *argv[] )
{
   int m, n, *TOTAL, *MAX, *ALLOC, i, j, k, nr, rt, req;
   FILE *fp;
   char fname[64];

   if (argc < 3) {
      fprintf(stderr, "Run with m (number of resources) and n (number of threads)\n");
      exit(1);
   }
   m = atoi(argv[1]);
   n = atoi(argv[2]);

   srand((unsigned int)time(NULL));

   TOTAL = (int *)malloc(m * sizeof(int));
   for (j=0; j<m; ++j) TOTAL[j] = 15 + rand() % 16;
   fp = (FILE *)fopen("input/system.txt", "w");
   fprintf(fp, "%d\n%d\n", m, n);
   for (j=0; j<m; ++j) fprintf(fp, "%d%c", TOTAL[j],  (j == m-1) ? '\n' : ' ');
   fclose(fp);

   MAX = (int *)malloc(m * sizeof(int));
   ALLOC = (int *)malloc(m * sizeof(int));
   for (i=0; i<n; ++i) {
      sprintf(fname, "input/thread%02d.txt", i);
      fp = (FILE *)fopen(fname,"w");
      nr = 5 + rand() % 6;
      fprintf(fp, "      ");
      for (j=0; j<m; ++j) {
         /* Use one of the following two lines */
         MAX[j] = rand() % (TOTAL[j] / 2);  /* Likely to create deadlock */
         // MAX[j] = rand() % (TOTAL[j] / 3);  /* Likely to create unsafe states without deadlock */
         ALLOC[j] = 0;
         fprintf(fp, "%3d%c", MAX[j], (j == m-1) ? '\n' : ' ');
      }
      for (k=0; k<nr; ++k) {
         fprintf(fp, "%2d  R", 5 + rand() % 36);
         for (j=0; j<m; ++j) {
            if (ALLOC[j] == 0) rt = 1;
            else if (ALLOC[j] == MAX[j]) rt = 0;
            else rt = rand() % 2;
            if (rt == 0) req = -(rand() % (1 + ALLOC[j]));
            else req = rand() % (1 + MAX[j] - ALLOC[j]);
            fprintf(fp, " %3d", req);
            ALLOC[j] += req;
         }
         fprintf(fp, "\n");
      }
      fprintf(fp, "%2d  Q\n", 5 + rand() % 36);
      fclose(fp);
   }

   exit(0);
}
