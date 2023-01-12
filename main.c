#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

struct process_edge_t {
    int childsCount;
    int childNodes[10];
    int signal;
};

enum Action { SEND, RECEIVE };

int root_pid;
int currentProcessIndex;
int total_sended = 0;
int total_received = 0;
int child_pids[10];
int child_group[10];
struct process_edge_t process_hierarhy[10];

void printInfo(int from, enum Action action, int signal) {
    struct timespec time;
    struct tm local_time;
    clock_gettime(CLOCK_REALTIME, &time);
    time_t sec = time.tv_sec;
    int ms = time.tv_nsec / 1e6;
    local_time = *localtime(&sec);

    printf("Process: %2d, %s %s [%d:%d:%d.%d]\n", from,
           action == RECEIVE ? "receive" : "send",
           signal == SIGUSR1 ? "SIGUSR1" : "SIGUSR2",
           local_time.tm_hour, local_time.tm_min, local_time.tm_sec, ms);
}

void user_signal_handler(int signo, siginfo_t *siginfo, void *context) {
    total_received++;
    printInfo(currentProcessIndex, RECEIVE, signo);
}

void term_handler(int _signo) {
    printf("Sigterm received: %d\n", currentProcessIndex);
    fflush(stdout);
    for (int i = 0; i < process_hierarhy[currentProcessIndex].childsCount; i++) {
        kill(child_pids[i], SIGTERM);
        waitpid(child_pids[i], NULL, 0);
    }
    printf("ID: %d, Total Sended: %d, Total Received: %d\n", currentProcessIndex, total_sended, total_received);
    exit(0);
}

int main() {
    struct sigaction user_signal_action;
    struct sigaction term_action;
    int child_process_count = 0;

    user_signal_action.sa_sigaction = &user_signal_handler;
    user_signal_action.sa_flags = SA_SIGINFO;

    sigaction(SIGUSR1, &user_signal_action, NULL);
    sigaction(SIGUSR2, &user_signal_action, NULL);

    term_action.sa_handler = &term_handler;
    sigemptyset(&term_action.sa_mask);
    sigaddset(&term_action.sa_mask, SIGUSR1);
    sigaddset(&term_action.sa_mask, SIGUSR2);

    sigaction(SIGTERM, &term_action, NULL);

    process_hierarhy[1] = (struct process_edge_t){
        .childsCount = 5, .signal = SIGUSR1,
        .childNodes = {2, 3, 4, 5, 6 }
    };

    process_hierarhy[6] = (struct process_edge_t){
        .childsCount = 2, .signal = SIGUSR2,
        .childNodes = { 7, 8 }
    };

    sleep(1);

    if (fork() == 0) {
        int need_spawn_childs;
        root_pid = getpid();
        currentProcessIndex = 1;
        do {
            child_process_count = 0;
            need_spawn_childs = 0;
            child_group[currentProcessIndex] = -1;
            for (int i = 0; i < process_hierarhy[currentProcessIndex].childsCount; i++) {
                int child_pid;
                if ((child_pid = fork()) == 0) {
                    currentProcessIndex = process_hierarhy[currentProcessIndex].childNodes[i];
                    need_spawn_childs = 1;
                    break;
                } else {
                    if (child_group[currentProcessIndex] == -1) {
                        child_group[currentProcessIndex] = child_pid;
                    }
                    setpgid(child_pid, child_group[currentProcessIndex]);
                    child_pids[child_process_count++] = child_pid;
                }
            }
        } while (need_spawn_childs);
    } else {
        sleep(1);
        wait(NULL);
        return 0;
    }

    while (1) {
        if (process_hierarhy[currentProcessIndex].signal != 0) {
            printInfo(currentProcessIndex, SEND, process_hierarhy[currentProcessIndex].signal);
            kill(-child_group[currentProcessIndex], process_hierarhy[currentProcessIndex].signal);
            total_sended++;
            if (currentProcessIndex == 1 && total_sended == 1001) {
                raise(SIGTERM);
            }
        } else if (currentProcessIndex == 8) {
            printInfo(currentProcessIndex, SEND, SIGUSR2);
            kill(root_pid, SIGUSR2);
            total_sended++;
        } else {
            pause();
        }
    }
}
