#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_SIZE 100

void merge(int arr[], int left[], int ls, int right[], int rs)
{
    int i = 0, j = 0, k = 0;
    while (i < ls && j < rs)
    {
        if (left[i] <= right[j]) arr[k++] = left[i++];
        else arr[k++] = right[j++];
    }
    while (i < ls) arr[k++] = left[i++];
    while (j < rs) arr[k++] = right[j++];
}

void merge_sort(int arr[], int size)
{
    if (size < 2) return; 

    int mid = size / 2;
    int left[mid], right[size - mid];

    for (int i = 0; i < mid; i++) left[i] = arr[i];
    for (int i = mid; i < size; i++) right[i - mid] = arr[i];

    int lfd[2], rfd[2];
    pipe(lfd);
    pipe(rfd);

    pid_t pid_left = fork();
    if (pid_left == 0)
    {
        close(lfd[0]);
        merge_sort(left, mid);
        write(lfd[1], left, mid * sizeof(int));
        close(lfd[1]);
        exit(0);
    }

    pid_t pid_right = fork();
    if (pid_right == 0)
    {
        close(rfd[0]);
        merge_sort(right, size - mid);
        write(rfd[1], right, (size - mid) * sizeof(int));
        close(rfd[1]);
        exit(0);
    }

    close(lfd[1]);
    close(rfd[1]);

    read(lfd[0], left, mid * sizeof(int));
    read(rfd[0], right, (size - mid) * sizeof(int));
    close(lfd[0]);
    close(rfd[0]);

    waitpid(pid_left, NULL, 0);
    waitpid(pid_right, NULL, 0);
    merge(arr, left, mid, right, size - mid);
}


int main()
{
    int n;
    int arr[MAX_SIZE];

    printf("Enter number of elements: ");
    scanf("%d", &n);

    printf("Enter elements:");
    for (int i = 0; i < n; i++) scanf("%d", &arr[i]);
    merge_sort(arr, n);

    printf("Sorted array: ");
    for (int i = 0; i < n; i++) printf("%d ", arr[i]);
    printf("\n");
    return 0;
}
