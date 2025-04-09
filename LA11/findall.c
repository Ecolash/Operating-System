/*
-------------------------------------------------------------------------------
ASSIGNMENT - 11 | File System Interface in Unix
-------------------------------------------------------------------------------
Name: Tuhin Mondal
Roll No: 22CS10087
-------------------------------------------------------------------------------
NOTE:

1) This code requires 2 arguments - directory path and file extension.
2) Some directories may not be accessible due to permission issues.
3) Example usage: 

    ./findall /tmp .log
    ./findall /home/tuhin/ .c
    ./findall ~ .txt
-------------------------------------------------------------------------------
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>

#define MAX_MAP         1024
#define MAX_LINE        512
#define MAX_PATHLEN     8192
#define MAX_LOGIN       256

static uid_t UIDs[MAX_MAP];
static char login[MAX_MAP][MAX_LOGIN];
static int mapsize = 0;

void map_UID()
{
    FILE *fp = fopen("/etc/passwd", "r");
    if (fp == NULL)
    {
        printf("[-] ERROR: Cannot open /etc/passwd: %s\n", strerror(errno));
        perror("fopen /etc/passwd");
        exit(EXIT_FAILURE);
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp) && mapsize < MAX_MAP)
    {
        char *p = line;
        char *colon = strchr(p, ':');
        if (!colon) continue;

        *colon = '\0';
        char *user = p;
        p = colon + 1;
        colon = strchr(p, ':');

        if (!colon) continue;
        p = colon + 1;

        colon = strchr(p, ':');
        if (colon) *colon = '\0';
        uid_t uid = (uid_t)atoi(p);

        strncpy(login[mapsize], user, MAX_LOGIN - 1);
        login[mapsize][MAX_LOGIN - 1] = '\0';
        UIDs[mapsize] = uid;
        mapsize++;
    }
    fclose(fp);
}

const char *login_info(uid_t uid)
{
    int i = 0;
    while(i < mapsize) 
    {
        if (UIDs[i] == uid) return login[i];
        i++;
    }
    return "unknown";
}

void dir_search(const char *dname, const char *ext, int *serial)
{
    DIR *dir = opendir(dname);
    if (dir == NULL)
    {
        fprintf(stderr, "ERROR: Cannot open directory '%s': %s\n", dname, strerror(errno));
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        if (strcmp(entry->d_name, ".") == 0)  continue;
        if (strcmp(entry->d_name, "..") == 0) continue;

        char path[MAX_PATHLEN];
        snprintf(path, sizeof(path), "%s/%s", dname, entry->d_name);

        struct stat statbuf;
        if (lstat(path, &statbuf) < 0)
        {
            fprintf(stderr, "[-] ERROR: Cannot stat '%s': %s\n", path, strerror(errno));
            continue;
        }

        if (S_ISDIR(statbuf.st_mode)) dir_search(path, ext, serial);
        else if (S_ISREG(statbuf.st_mode))
        {
            char *dot = strrchr(entry->d_name, '.');
            if (dot == NULL) continue;
            if (*(dot + 1) == '\0') continue;
            if (strcmp(dot + 1, ext)) continue;
            
            ++(*serial);
            printf("%-6d : ", *serial);
            printf("%-25s", login_info(statbuf.st_uid));
            printf("%-18ld", (long)statbuf.st_size);
            printf("%-100s", path);
            printf("\n");
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[])
{
    char dname[100];
    char extns[100];
    if (argc != 3)
    {
        printf("[-] Usage: %s <directory> <extension>\n", argv[0]);
        printf("\t\t <directory> : Directory to search in\n");
        printf("\t\t <extension> : File extension to search for\n");
        exit(EXIT_FAILURE);
    }

    strcpy(dname, argv[1]);
    strcpy(extns, argv[2]);
    map_UID();

    printf("%-6s : %-25s %-18s %s\n", "NO", "OWNER", "SIZE", "NAME");
    printf("%-7s ", "--");
    printf("%-26s ", "-----");
    printf("%-18s ", "----");
    printf("%s\n", "----");

    int serial = 0;
    dir_search(dname, extns, &serial);
    printf("\n");

    printf("+++ %d files found with extension %s\n", serial, extns);
    return 0;
}