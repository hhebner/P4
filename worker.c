#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    long mtype;
    char mtext[100];
} message_buf;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Message queue ID argument missing.\n");
        exit(EXIT_FAILURE);
    }

    int msgid = atoi(argv[1]);  // Convert string argument back to integer
    message_buf msg;

    // Receive a message from oss.c
    int rcv_status = msgrcv(msgid, &msg, sizeof(msg), 1, 0);
    if (rcv_status == -1) {
        perror("msgrcv failed");
        exit(EXIT_FAILURE);
    }
    printf("Worker (PID: %d) received: %s\n", getpid(), msg.mtext);

    // Send a message back to oss.c
    msg.mtype = 2;  // Assuming a different message type for communication back to oss.c
    strcpy(msg.mtext, "Message from Worker");
    if (msgsnd(msgid, &msg, sizeof(msg), 0) == -1) {
        perror("msgsnd failed");
        exit(EXIT_FAILURE);
    }

    return 0;
}
