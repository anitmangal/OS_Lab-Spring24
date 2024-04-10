#define _DEFAULT_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>

#define MAX_PATH_LEN 1024

// Function to check if the attributes of the files are the same
void checkattr(char * src, char * dst) {
    if (strcmp(src, ".") == 0 || strcmp(src, "..") == 0) return;        // Skip the current and parent directory

    DIR * dir_src = opendir(src);
    if (dir_src == NULL) {
        fprintf(stderr, "Error: cannot open directory\n");
        exit(1);
    }
    struct dirent * entry_src;
    // Iterate through the files in the source directory
    while ((entry_src = readdir(dir_src)) != NULL) {
        if (strcmp(entry_src->d_name, ".") == 0 || strcmp(entry_src->d_name, "..") == 0) continue;
        char dstfilename[MAX_PATH_LEN];
        sprintf(dstfilename, "%s/%s", dst, entry_src->d_name);
        char srcfilename[MAX_PATH_LEN];
        sprintf(srcfilename, "%s/%s", src, entry_src->d_name);

        if (entry_src->d_type == DT_DIR) checkattr(srcfilename, dstfilename);       // Recursively check the attributes of the files in the directory

        // Check if the attributes of the files are the same
        struct stat srcstat;
        struct stat dststat;
        stat(srcfilename, &srcstat);
        stat(dstfilename, &dststat);
        if (srcstat.st_mode != dststat.st_mode) {
            // Change the mode of the file (permissions)
            chmod(dstfilename, srcstat.st_mode);
            printf("[p] %s\n", dstfilename);
        }
        if (srcstat.st_mtime != dststat.st_mtime || (entry_src->d_type != DT_DIR && srcstat.st_atime != dststat.st_atime)) {
            // Change the timestamps of the file
            struct utimbuf times;
            times.actime = srcstat.st_atime;
            times.modtime = srcstat.st_mtime;
            if (utime(dstfilename, &times) != 0) {
                fprintf(stderr, "Error: cannot set time\n");
                exit(1);
            }
            printf("[t] %s\n", dstfilename);
        }
    }
    closedir(dir_src);
}

// Function to recursively delete the files in the directory
void recdelete(char* dst) {
    DIR * dir = opendir(dst);
    struct dirent * entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        char dstfilename[MAX_PATH_LEN];
        sprintf(dstfilename, "%s/%s", dst, entry->d_name);

        if (entry->d_type == DT_DIR) {
            // Recursively delete the files in the directory
            recdelete(dstfilename);
            rmdir(dstfilename);
        }
        else {
            // Delete the file
            remove(dstfilename);
        }
        printf("[-] %s\n", dstfilename);
    }
    closedir(dir);
}

// Function to synchronize the files in the source and destination directories
void check(char * src, char * dst) {
    if (strcmp(src, ".") == 0 || strcmp(src, "..") == 0) return;

    DIR * dir_src = opendir(src);
    if (dir_src == NULL) {
        fprintf(stderr, "Error: cannot open directory\n");
        exit(1);
    }

    struct dirent * entry_src;
    // Iterate through the files in the source directory
    while ((entry_src = readdir(dir_src)) != NULL) {
        if (strcmp(entry_src->d_name, ".") == 0 || strcmp(entry_src->d_name, "..") == 0) continue;

        // Check if the file is in the destination directory
        DIR * dir_dst = opendir(dst);
        struct dirent * entry_dst;
        int found = 0;
        while ((entry_dst = readdir(dir_dst)) != NULL) {
            if (strcmp(entry_src->d_name, entry_dst->d_name) == 0) {
                found = 1;
                break;
            }
        }

        char dstfilename[MAX_PATH_LEN];
        sprintf(dstfilename, "%s/%s", dst, entry_src->d_name);
        char srcfilename[MAX_PATH_LEN];
        sprintf(srcfilename, "%s/%s", src, entry_src->d_name);
        if (found == 0) {
            // Copy the file to the destination directory

            if (entry_src->d_type == DT_DIR) {
                // Create a new directory, recursively copy the files in the directory
                mkdir(dstfilename, 0777);
                printf("[+] %s\n", dstfilename);
                check(srcfilename, dstfilename);
            }
            else {
                // Copy the file
                FILE * srcfile = fopen(srcfilename, "r");
                if (srcfile == NULL) {
                    fprintf(stderr, "Error: cannot open src file\n");
                    exit(1);
                }
                FILE * dstfile = fopen(dstfilename, "w");
                if (dstfile == NULL) {
                    fprintf(stderr, "Error: cannot open dest file\n");
                    exit(1);
                }
                char buffer[MAX_PATH_LEN];
                size_t n;
                while ((n = fread(buffer, 1, sizeof(buffer), srcfile)) > 0) {
                    fwrite(buffer, 1, n, dstfile);
                }
                fclose(srcfile);
                fclose(dstfile);
                printf("[+] %s\n", dstfilename);
            }
            // Set the mode and timestamps of the file
            struct stat srcstat;
            stat(srcfilename, &srcstat);
            chmod(dstfilename, srcstat.st_mode);
            struct utimbuf times;
            times.actime = srcstat.st_atime;
            times.modtime = srcstat.st_mtime;
            utime(dstfilename, &times);
            closedir(dir_dst);
        }
        else {
            // Check if the file in the destination directory is the same as the file in the source directory, if not, overwrite the file
            if (entry_src->d_type == DT_DIR) {
                check(srcfilename, dstfilename);
            }
            else {
                struct stat srcstat;
                struct stat dststat;
                stat(srcfilename, &srcstat);
                stat(dstfilename, &dststat);
                if (srcstat.st_mtime != dststat.st_mtime || srcstat.st_size != dststat.st_size) {
                    FILE * srcfile = fopen(srcfilename, "r");
                    FILE * dstfile = fopen(dstfilename, "w");
                    char buffer[MAX_PATH_LEN];
                    size_t n;
                    while ((n = fread(buffer, 1, sizeof(buffer), srcfile)) > 0) {
                        fwrite(buffer, 1, n, dstfile);
                    }
                    fclose(srcfile);
                    fclose(dstfile);
                    struct stat srcstat;
                    stat(srcfilename, &srcstat);
                    chmod(dstfilename, srcstat.st_mode);
                    struct utimbuf times;
                    times.actime = srcstat.st_atime;
                    times.modtime = srcstat.st_mtime;
                    utime(dstfilename, &times);
                    closedir(dir_dst);
                    printf("[o] %s\n", dstfilename);
                }
            }
        }
    }
    
    closedir(dir_src);

    // Check if there are any files in the destination directory that are not in the source directory
    DIR * dir_dst = opendir(dst);
    struct dirent * entry_dst;
    while ((entry_dst = readdir(dir_dst)) != NULL) {
        if (strcmp(entry_dst->d_name, ".") == 0 || strcmp(entry_dst->d_name, "..") == 0) continue;
        DIR * dir_src = opendir(src);
        struct dirent * entry_src;
        int found = 0;
        while ((entry_src = readdir(dir_src)) != NULL) {
            if (strcmp(entry_src->d_name, entry_dst->d_name) == 0) {
                found = 1;
                break;
            }
        }

        char dstfilename[MAX_PATH_LEN];
        sprintf(dstfilename, "%s/%s", dst, entry_dst->d_name);
        char srcfilename[MAX_PATH_LEN];
        sprintf(srcfilename, "%s/%s", src, entry_dst->d_name);
        if (found == 0) {
            // Delete the file in the destination directory
            if (entry_dst->d_type == DT_DIR) {
                recdelete(dstfilename);
                rmdir(dstfilename);
            }
            else {
                remove(dstfilename);
            }
            printf("[-] %s\n", dstfilename);
        }
        closedir(dir_src);
    }
    closedir(dir_dst);
}

int main(int argc, char * argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <src path> <dst path>\n", argv[0]);
        exit(1);
    }

    check(argv[1], argv[2]);
    checkattr(argv[1], argv[2]);
    exit(0);
}