#include "kernel/types.h"
#include "user.h"

int main(void)
{
    int pid_child, pid_parent;
    int parent_pp[2], child_pp[2]; //0 -> read; 1 -> write;
    int ret_p = pipe(parent_pp);
    int ret_c = pipe(child_pp);

    /* Check parameters */
    if ((pid_child = fork()) < 0)
    {
        printf("fork Error!");
    }
    if (ret_p == -1)
    {
        printf("pipe Error!");
    }
    if (ret_c == -1)
    {
        printf("pipe Error!");
    }

    /* Pipe */
    char buf[20];

    if (pid_child == 0) //child
    {
        pid_child = getpid();
        read(parent_pp[0], buf, 4); //the data read -> buf
        printf("%d: received %s\n", pid_child, buf);
        write(child_pp[1], "pong", strlen("pong"));
    }
    else //parent
    {
        pid_parent = getpid();
        write(parent_pp[1], "ping", strlen("ping"));
        read(child_pp[0], buf, 4); //the data read -> buf
        printf("%d: received %s\n", pid_parent, buf);
    }

    /* eixt */
    wait(0);
    close(parent_pp[0]);
    close(parent_pp[1]);
    close(child_pp[0]);
    close(child_pp[1]);
    exit(0);
}