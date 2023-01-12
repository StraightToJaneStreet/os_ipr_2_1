#define _GNU_SOURCE

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>

int child;
int state = 0;

volatile int can_exit = 0;

volatile int delivery_success_counter = 0;

struct sigaction child_action;

void handler(int _signo) {
    printf("Signal handled\n");
    kill(child, SIGALRM);
}

void delivery_success_handler(int no) {
    delivery_success_counter--;
}

void termination_handler(int no) {
    can_exit = 1;
}

int main() {
    printf("Hello world\n");

    struct sigaction action;
    action.sa_handler = &handler;
    action.sa_flags = SA_NODEFER;

    struct sigaction term_action;
    term_action.sa_handler = termination_handler;

    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGTERM, &term_action, NULL);
    child_action.sa_handler = &delivery_success_handler;
    child_action.sa_flags = SA_NODEFER;


    if ((child = fork()) == 0) {
        int parent_pid = getppid();
        sigaction(SIGALRM, &child_action, NULL);

        for (int i = 0; i < 100; i++) {
            while (delivery_success_counter != 0) {
                pause();
            }
            delivery_success_counter = 1;
            kill(parent_pid, SIGUSR1);
        }

        kill(parent_pid, SIGTERM);
    } else  {
        sigset_t waiting;
        sigemptyset(&waiting);
        //sigaddset(&waiting, SIGUSR1);

        while (can_exit != 1) {
            pause();
        }
    }

    return 0;
}