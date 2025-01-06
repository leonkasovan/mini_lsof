#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#define MAX_PATH 1024

// Function to list open file descriptors for a given PID
void list_open_files(int pid) {
    char path[MAX_PATH];
    char link_target[MAX_PATH];
    struct dirent *entry;
    DIR *dir;
    struct stat file_stat;

    // Build the path to the /proc/[pid]/fd/ directory
    snprintf(path, sizeof(path), "/proc/%d/fd/", pid);

    // Open the directory
    dir = opendir(path);
    if (dir == NULL) {
        perror("Failed to open directory");
        return;
    }

    printf("Open files for process ID %d:\n", pid);
    printf("%-5s %-10s %-20s\n", "FD", "Type", "File Path");

    // Read each entry in the /proc/[pid]/fd/ directory
    while ((entry = readdir(dir)) != NULL) {
        // Skip '.' and '..'
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Build the full path to the file descriptor
        snprintf(path, sizeof(path), "/proc/%d/fd/%s", pid, entry->d_name);

        // Read the symbolic link to get the file path
        ssize_t len = readlink(path, link_target, sizeof(link_target) - 1);
        if (len == -1) {
            perror("Failed to read symbolic link");
            continue;
        }
        link_target[len] = '\0';  // Null-terminate the string

        // Check if the file is a pipe or socket (do not stat them)
        if (strstr(link_target, "pipe:") != NULL) {
            printf("%-5s %-10s %-20s\n", entry->d_name, "Pipe", link_target);
            continue;
        }
        if (strstr(link_target, "socket:") != NULL) {
            printf("%-5s %-10s %-20s\n", entry->d_name, "Socket", link_target);
            continue;
        }

        // Get the file status using stat() to determine the file type
        if (stat(link_target, &file_stat) == -1) {
            perror("Failed to stat file");
            continue;
        }

        // Determine file type
        char file_type[20] = "Unknown";
        if (S_ISREG(file_stat.st_mode)) {
            strcpy(file_type, "Regular File");
        } else if (S_ISDIR(file_stat.st_mode)) {
            strcpy(file_type, "Directory");
        } else if (S_ISCHR(file_stat.st_mode)) {
            strcpy(file_type, "Character Device");
        } else if (S_ISBLK(file_stat.st_mode)) {
            strcpy(file_type, "Block Device");
        } else if (S_ISFIFO(file_stat.st_mode)) {
            strcpy(file_type, "FIFO (Named Pipe)");
        } else if (S_ISSOCK(file_stat.st_mode)) {
            strcpy(file_type, "Socket");
        }

        // Print the file descriptor number, file type, and file path
        printf("%-5s %-10s %-20s\n", entry->d_name, file_type, link_target);
    }

    // Close the directory
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
        return 1;
    }

    int pid = atoi(argv[1]);
    if (pid <= 0) {
        fprintf(stderr, "Invalid PID\n");
        return 1;
    }

    // List open files for the specified PID
    list_open_files(pid);

    return 0;
}