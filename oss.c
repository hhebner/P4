#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/wait.h>

typedef struct PCB {
        int occupied;
        pid_t pid;
        int startSeconds;
        int startNano;
        int serviceTimeSeconds;
        int serviceTimeNano;
        int eventWaitSec;
        int eventWaitNano;
        int blocked;
} ProcessControlBlock;

ProcessControlBlock pct[20];

typedef struct {
        long mtype;
        char mtext[100];
} Message;

typedef struct {
        unsigned int seconds;
        unsigned int nano_seconds;
} SysClock;

void incrementClock(SysClock* clock, unsigned int seconds, unsigned int nano_seconds) {
        clock->nano_seconds += nano_seconds;

        while (clock->nano_seconds >= 1000000000) {
                clock->seconds += 1;
                clock->nano_seconds -= 1000000000;
        }
        clock->seconds += seconds;
}

int allocatePCB() {
        for (int i = 0; i < 20; i++) {
                if (pct[i].occupied == 0) {
                        pct[i].occupied = 1;
                        return i;
                }
        }
        return -1;
}
int main(int argc, char* argv[]) {
        int opt;
        int n = 1;
        int s = 1;
        int t = 1000000;
        char *f = "logfile.txt";

        // Parse command-line arguments
        while ((opt = getopt(argc, argv, "hn:s:t:f:")) != -1) {
                switch (opt) {
                   case 'h':
                        printf("Usage: %s [-h] [-n proc] [-s simul] [-t timeToLaunchNewChild] [-f logfile]\n", argv[0]);
                        exit(EXIT_SUCCESS);
                        break;
                   case 'n':
                        n = atoi(optarg);
                        break;
                   case 's':
                        s = atoi(optarg);
                        break;
                   case 't':
                        t = atoi(optarg);
                        break;
                   case 'f':
                        f = optarg;
                        break;
                   default:  // option not recognized
                        fprintf(stderr, "Usage: %s [-h] [-n proc] [-s simul] [-t timeToLaunchNewChild] [-f logfile]\n", argv[0]);
                        exit(EXIT_FAILURE);
                        break;
                }
        }


        printf("-n value = %d\n", n;
        printf("-s value = %d\n", s);
        printf("-t value = %d\n", t);
        printf("-f value = %s\n", f);

        key_t key = 290161;
        int shmid;
        SysClock* shm_clock;
        int msgid;
        Message msg;

        if ((shmid = shmget(key, sizeof(SysClock), IPC_CREAT | 0666)) < 0) {
                perror("shmget failed");
                exit(1);
        }

        if ((shm_clock = (SysClock*) shmat(shmid, NULL, 0)) == (SysClock *)-1) {
                perror("shmat failed");
                exit(1);
        }

        if ((msgid = msgget(IPC_PRIVATE, 0666 | IPC_CREAT)) == -1) {
                perror("Message queue failed to create");
                exit(EXIT_FAILURE);
        }

        shm_clock->seconds = 0;
        shm_clock->nano_seconds = 0;

        pid_t fork_pid;

        for (int i = 0; i < 20; i++) {
                int index = allocatePCB();
                if (index == -1) {
                        printf("No space available in PCB\n");
                        break;
                }


                fork_pid = fork();

                if (fork_pid < 0) {
                        perror("Fork failed");
                        exit(EXIT_FAILURE);
                } else if (fork_pid == 0) {
                        pct[index].pid = getpid();
                        char msg_id_str[20];
                        sprintf(msg_id_str, "%d", msgid);

                        execl("./worker", "./worker", msg_id_str, NULL);
                } else {
                        sleep(1);
                        msg.mtype = 1;
                        strcpy(msg.mtext, "Message from Master");
                        msgsnd(msgid, &msg, sizeof(msg), 0);
                        printf("Master sent: %s\n", msg.mtext);

                        int rcv_status = msgrcv(msgid, &msg, sizeof(msg), 2, 0);
                        if (rcv_status == -1) {
                                perror("msgrcv failed");
                        } else {
                                printf("Master received: %s\n", msg.mtext);
                        }

                }
        }

        for (int i = 0; i < 20; i++) {
                if (pct[i].occupied == 1) {
                        waitpid(pct[i].pid, NULL, 0);
                        pct[i].occupied = 0;
                }
        }

        if (fork_pid != 0) {
                msgctl(msgid, IPC_RMID, NULL);
        }

        if (shmdt(shm_clock) == -1) {
                perror("shmdt failed");
                exit(1);
        }

        if (shmctl(shmid, IPC_RMID, NULL) == -1) {
                perror("shmctl failed");
                exit(1);
        }

         return 0;
}
