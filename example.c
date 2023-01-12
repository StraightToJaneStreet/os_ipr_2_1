#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <wait.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>

#define PAIRS_COUNT 9
#define PROCESSES_COUNT 9
#define NOSIG -1
#define SIGDEF -2

typedef struct tagPair {
    int create;
    int signal;
    int from;
    int to;
} Pair;

Pair pairs[] = {
        {1, NOSIG, 0, 1},
        {1, SIGUSR2, 1, 2},
        {1, SIGUSR2, 1, 3},
        {1, SIGUSR2, 1, 4},
        {1, SIGUSR2, 1, 5},
        {1, SIGUSR1, 2, 6},
        {1, SIGUSR1, 3, 7},
        {1, SIGUSR1, 4, 8},
        {0, SIGUSR1, 8, 1}
};

pid_t pids[PROCESSES_COUNT];
int nodeIndex;
int startNodeIndex;

int sendedSignalsCount[2];

void sigusr(int);
void sigterm(int);
int signalChildren(int index, int sig);
void outputInfo(char* status, int sigNum);
void printHeader();

int main() {
    pids[0] = getpid();

    struct sigaction signalAction;
    signalAction.sa_flags = SA_SIGINFO;

    int pgid = 0;

    for (int i = 0; i < PAIRS_COUNT; ++i) {
        if (i == 0) {
            pgid = 0;
        }

        if (nodeIndex == pairs[i].to && pairs[i].signal != NOSIG) {
            if (!startNodeIndex)
                startNodeIndex = pairs[i].to;
            signal(pairs[i].signal, sigusr);
        }

        if (nodeIndex == pairs[i].from && pairs[i].create) {
            pid_t pid;
            switch (pid = fork()) {
                case -1: {
                    break;
                }
                case 0: {
                    nodeIndex = pairs[i].to;
                    pids[nodeIndex] = getpid();
                    i = -1;

                    signal(SIGTERM, sigterm);
                    break;
                }
                default: {
                    if (!pgid) {
                        pgid = pid;
                    }
                    setpgid(pid, pgid);
                    pids[pairs[i].to] = pid;
                    break;
                }
            }
        }
    }

    if (nodeIndex == 0) {
        printHeader();
    }

    sleep(1);

    if (nodeIndex == startNodeIndex) {
        int sig = signalChildren(nodeIndex, SIGDEF);
        if (sig != NOSIG) {
            int sigNum = sig == SIGUSR1 ? 1 : 2;
            sendedSignalsCount[sigNum - 1]++;
        }
    }

    if (nodeIndex == 0) {
        getchar();
        while (wait(NULL) != -1);
        return 0;
    }
    else {
        while (1) {
            pause();
        }
    }
}

void sigusr(int sig) {
    outputInfo("get", sig == SIGUSR1 ? 1 : 2);

    if (nodeIndex == 1 && (sendedSignalsCount[0] + sendedSignalsCount[1]) == 101) {
        kill(getpid(), SIGTERM);
        return;
    }

    int signal = signalChildren(nodeIndex, SIGDEF);
    if (signal != NOSIG) {
        int sigNum = signal == SIGUSR1 ? 1 : 2;
        outputInfo("put", sigNum);
        sendedSignalsCount[sigNum - 1]++;
    }
}

int tryToGetChildrenPGID(int index, pid_t* pgid, int* sig) {
    for (int i = 0; i < PAIRS_COUNT; ++i) {
        if (pairs[i].from == index) {
            *pgid = getpgid(pids[pairs[i].to]);
            if (sig != NULL) {
                *sig = pairs[i].signal;
            }
            return 1;
        }
    }

    return 0;
}

int signalChildren(int index, int sig) {
    int s;
    pid_t pgid;
    if (tryToGetChildrenPGID(index, &pgid, &s)) {
        int signal = sig == SIGDEF ? s : sig;
        kill(-pgid, signal);
        return signal;
    }

    return NOSIG;
}

void printHeader() {
    printf("INDEX PID   PPID  STATUS SIGNAL TIME\n");
}

void outputInfo(char* status, int sigNum) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    time_t sec = ts.tv_sec;
    int ms = ts.tv_nsec / 1e6;

    struct tm* tmp = localtime(&sec);

    printf("%-5d %-5d %-5d %6s   USR%1d %02i:%02i:%02i:%03i\n",
           nodeIndex, getpid(), getppid(), status, sigNum,
           tmp->tm_hour, tmp->tm_min, tmp->tm_sec, ms);
    fflush(stdout);
}

void sigterm(int) {
    printf("%-5d %-5d %-5d has terminated. Sended: %4d SIGUSR1 and %4d SIGUSR2\n",
           nodeIndex, getpid(), getppid(), sendedSignalsCount[0], sendedSignalsCount[1]);

    pid_t pgid;
    if (tryToGetChildrenPGID(nodeIndex, &pgid, NULL)) {
        kill(-pgid, SIGTERM);
        waitpid(-pgid, NULL, 0);
    }

    exit(0);
}
