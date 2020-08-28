/* Client for file server using System V message queues */
#include "46_svmsg_file.h"

static int clientId;

static void removeQueue(void)
{
    if (msgctl(clientId, IPC_RMID, NULL) == -1)
        errExit("msgctl");
}

int main(int argc, char* argv[])
{
    struct requestMsg req;
    struct responseMsg resp;
    int serverId, numMsgs;
    ssize_t msgLen, totBytes;

    if (argc != 2 || strcmp(argv[1], "--help") == 0)
        usageErr("%s pathname\n", argv[0]);

    if (strlen(argv[1]) > sizeof(req.pathname) - 1)
        cmdLineErr("Pathname too long");

    /* Get server's queue identifier; create queue for response */
    serverId = msgget(SERVER_KEY, S_IWUSR);
    if (serverId == -1)
        errExit("msgget-- server message queue");

    clientId = msgget(IPC_PRIVATE, S_IRUSR | S_IWUSR | S_IWGRP);
    if (clientId == -1)
        errExit("msgget - client message queue");
    if (atexit(removeQueue) != 0)
        errExit("atexit");

    /* Send message asking for file named in argv[1] */
    req.mtype = 1;   /* Any type will do */
    req.clientId = clientId;
    strncpy(req.pathname, argv[1], sizeof(req.pathname) -1 );
    req.pathname[sizeof(req.pathname) - 1] ='\0';

    if (msgsnd(serverId, &req, REQ_MSG_SIZE, 0) == -1)
        errExit("msgsnd");

    /* Get first response, which may be failure notification */
    msgLen = msgrcv(clientId, &resp, RESP_MSG_SIZE, 0, 0);
    if (msgLen == -1)
        errExit("msgrcv");

    if (resp.mtype == RESP_MT_FAILURE)
    {
        printf("%s\n", resp.data);
        if (msgctl(clientId, IPC_RMID, NULL) == -1)
            errExit("msgctl");
        exit(EXIT_FAILURE);
    }

    totBytes = msgLen;
    for (numMsgs = 1; resp.mtype == RESP_MT_DATA; numMsgs++)
    {
        msgLen = msgrcv(clientId, &resp, RESP_MSG_SIZE, 0, 0);
        if (msgLen == -1)
            errExit("msgrcv");

        totBytes += msgLen;
    }

    printf("Received %ld bytes (%d messages)\n", (long)totBytes, numMsgs);

    exit(EXIT_SUCCESS);
}
