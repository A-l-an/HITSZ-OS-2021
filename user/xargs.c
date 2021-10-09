#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"
#define MAXARG 32

int main(int argc, char *argv[])
{
    int i, len, l;
    int j = 0, m = 0;
    char args_i[MAXARG];     //输入参数的临时储存区
    char args_i_ed[MAXARG];  //作一行命令行参数的临时储存区(翻译后)
    char *p = args_i_ed;     //指针，方便最后合并移动。
    char *argv_plus[MAXARG]; //最终合并后的命令行参数

    /*******把本命令的参数全部录入*******/
    for (i = 1; i < argc; i++, j++)
    {
        //第0个为xargs，第1个才是后一条该条执行指令本身。
        argv_plus[j] = argv[i];
    }

    /*******将输入参数翻译并录入进argv_plus*******/
    //len:输入的作为命令行参数的字符串的长度
    if ((len = read(0, args_i, sizeof(args_i))) > 0)
    {
        /* 对每一个输入的字符作处理 */
        for (l = 0; l < len; l++)
        {
            ///////准备读取另一个输入行//////////
            if (args_i[l] == '\n')
            {
                args_i_ed[m] = '\0';
                argv_plus[j] = p; //将翻译的这行命令行参数接到后一条指令的后面
                j++;
                argv_plus[j] = 0; //最后一个参数argv[size-1]必须为0，否则将执行失败。
                //重新初始化新的输入行//
                j = argc - 1;  //恢复到该指令本身的参数个数
                p = args_i_ed; //重新指向第一个元素
                m = 0;
                //真正执行过程//
                if (fork() == 0)
                {
                    exec(argv[1], argv_plus);
                }
                wait(0);
            }
            ///////空格转义符//////////
            else if (args_i[l] == ' ')
            {
                args_i_ed[m] = '\0';
                m++;
                argv_plus[j] = p;
                j++;
                p = &args_i_ed[m]; //指针移动
            }
            ///////正常读入字符，放入缓冲区//////////
            else
            {
                args_i_ed[m] = args_i[l];
                m++;
            }
        }
    }
    else
    {
        printf("Error!\n");
    }
    exit(0);
}
