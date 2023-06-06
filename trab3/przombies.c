#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>

#define MAX_LINE_LENGTH 512


void log_zombies(FILE *log_file) {
    DIR *proc_dir = opendir("/proc");
    struct dirent *entry;
    
    if (proc_dir == NULL) {
        fprintf(stderr, "Failed to open /proc directory\n");
        exit(1);
    }
    
    fprintf(log_file, "PID | PPID | Nome do Programa\n");
    fprintf(log_file, "==========================================\n");
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR && atoi(entry->d_name) != 0) {
            char status_path[MAX_LINE_LENGTH];
            FILE *status_file;
            char line[MAX_LINE_LENGTH];
            char name[MAX_LINE_LENGTH];
            int pid, ppid;
            
            snprintf(status_path, sizeof(status_path), "/proc/%s/status", entry->d_name);
            
            status_file = fopen(status_path, "r");
            if (status_file == NULL) {
                continue;
            }
            
            while (fgets(line, sizeof(line), status_file) != NULL) {
                if (strncmp(line, "Name:", 5) == 0) {
                    sscanf(line, "%*s %s", name);
                }
                else if (strncmp(line, "Pid:", 4) == 0) {
                    sscanf(line, "%*s %d", &pid);
                }
                else if (strncmp(line, "PPid:", 5) == 0) {
                    sscanf(line, "%*s %d", &ppid);
                }
            }
            
            fclose(status_file);
            
            if (pid != ppid) { // Zombie process condition
                fprintf(log_file, "%d | %d | %s\n", pid, ppid, name);
            }
        }
    }
    
    fprintf(log_file, "==========================================\n");
    
    closedir(proc_dir);
}

void handle_signal(int signal) {
    if (signal == SIGTERM) {
        FILE *log_file = fopen("zombie.log", "a");
        if (log_file == NULL) {
            fprintf(stderr, "Failed to open log file\n");
            exit(1);
        }
        
        fprintf(log_file, "Daemon terminated\n");
        fclose(log_file);
        
        exit(0);
    }
}

int main(int argc, char **argv) {
    int interval;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <interval>\n", argv[0]);
        return 1;
    }
    
    interval = atoi(argv[1]);
    if (interval <= 0) {
        fprintf(stderr, "Interval must be a positive integer\n");
        return 1;
    }
    
    if (fork() != 0) {
        exit(0); // Parent process terminates
    }
    
    setsid(); // Create a new session
    
    signal(SIGTERM, handle_signal);
    
    FILE *log_file = fopen("zombie.log", "w");
    if (log_file == NULL) {
        fprintf(stderr, "Failed to open log file\n");
        return 1;
    }
    
    while (1) {
        log_zombies(log_file);
        fflush(log_file);
        sleep(interval);
    }
    
    fclose(log_file);
    
    return 0;
}
