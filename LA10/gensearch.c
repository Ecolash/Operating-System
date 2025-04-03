#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main ( int argc, char *argv[] )
{
   int n, m, s, i, j;
   FILE *fp;

   srand((unsigned int)time(NULL));

   n = (argc == 1) ? 200 : atoi(argv[1]);
   m = (argc <= 2) ? 100 : atoi(argv[2]);

   fp = (FILE *)fopen("search.txt", "w");
   fprintf(fp, "%d %d\n", n, m);
   for (i=0; i<n; ++i) {
      s = 1000000 + rand() % 1000001;
      fprintf(fp, "%-10d ", s);
      for (j=0; j<m; ++j)
         fprintf(fp, "%-7d%c", rand() % s, (j == m-1) ? '\n' : ' ');
   }
   fclose(fp);

   exit(0);
}
