/* Prime sieve*/
//思路：从小到大递归求出素数，以此类推直至最大数。//

#include "kernel/types.h"
#include "user.h"
#define max 34
//查询数组的元素最大值减一赋给max。
//  例：如果查询2-35的所有质数，则赋34

/* 迭代进行素数的筛选 */
void Prime(int *data, int num)
{
    /*迭代的底层条件*/
    //必须放在第一位置，如满足则不执行以下步骤。
    if (num == 1)
    {
        printf("prime %d\n", *data);
        return;
    }

    int i;
    int pipe_fd[2];
    int ret = pipe(pipe_fd);

    //添加这句会导致重新创造一个子进程（？）致使程序完全出错
    // if ((pid = fork()) < 0){
    //     printf("fork Error!");}

    /* Check parameters */
    if (ret == -1)
    {
        printf("pipe Error!");
    }

    int temp;          //存储写入数字的中间变量
    int prime = *data; //每次迭代的第一个数必是素数

    printf("prime %d\n", prime);

    /* Child1 - write */
    if (fork() == 0)
    {
        for (i = 0; i < num; i++) //先把整个数组(筛选后)都写进管道
        {
            temp = *(data + i);
            write(pipe_fd[1], (char *)(&temp), 4); //4个字节、32位
        }
        exit(0); //运行完就退出进程，节省资源。
    }
    close(pipe_fd[1]); //子进程结束了才能关写通道（why）

    /* Child2 - read */
    if (fork() == 0)
    {
        int cnt = 0;
        char buf_o[4];
        /* 只要读不为空，则一个个校验元素 */
        while (read(pipe_fd[0], buf_o, 4) != 0)
        {
            temp = *((int *)buf_o); //强转为int型再解引用
            /* 筛选步骤 */
            if (temp % prime != 0)
            {
                *data = temp; //重写数组(即筛出非素数)
                data++;       //下一个元素地址
                cnt++;
            }
        }
        Prime(data - cnt, cnt); //迭代(不可放在子进程之外)
        exit(0);
    }
    close(pipe_fd[0]);
    /* 回收产生的两个子进程 确保子进程先退出，父进程再退出,否则子程序还在运行会导致$号出现 */
    //why
    wait(0);
    wait(0);
}

int main(void)
{
    int data[max]; //存储2—35
    for (int i = 0; i < max; i++)
    {
        data[i] = i + 2;
    }
    Prime(data, max); //传指针型数组，方便后续针对特定的元素进行修改。
    exit(0);
}
