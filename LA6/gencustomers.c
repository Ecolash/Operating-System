#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main ( )
{
   int i, t, c, n;

   srand((unsigned int)time(NULL));

   t = n = 0;

   i = 7;
   while (i) {
      ++n;
      if (rand() % 2) c = 1;
      else if (rand() % 2) c = 2;
      else c = (rand() % 2) ? 3 : 4;
      printf("%d %d %d\n", n, t, c);
      --i;
   }

   while (1) {
      ++n;
      t += rand() % 10;
      if (rand() % 2) c = 1;
      else if (rand() % 2) c = 2;
      else c = (rand() % 2) ? 3 : 4;
      printf("%d %d %d\n", n, t, c);
      if (t > 250) break;
   }
   printf("-1\n");

   exit(0);
}
