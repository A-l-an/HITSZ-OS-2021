#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

/* 返回文件名字 */
char *
fmtname(char *path)
{
    static char buf[DIRSIZ + 1];
    char *p;

    // Find first character after last slash.
    for (p = path + strlen(path); p >= path && *p != '/'; p--)
        ;
    p++;

    // Return blank-padded name.
    //有这些会导致有空格，字符串对比的时候失败。
    // if (strlen(p) >= DIRSIZ)
    //     return p;
    memmove(buf, p, strlen(p) + 1); //还得+1才能解码（？）成功
    // memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));
    return buf;
}

void find(char *path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;

    /* 检查元素 */
    if ((fd = open(path, 0)) < 0)
    {
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if (fstat(fd, &st) < 0)
    {
        fprintf(2, "find: cannot stat %s\n", path);
        close(fd);
        return;
    }

    /********分类别迭代显示********/
    switch (st.type)
    {
    /*类型是文件时*/
    case T_FILE:
        /* 文件名相同 则打印完整的路径 */
        if (strcmp(fmtname(path), name) == 0) //匹配成功
            printf("%s\n", path);
        break;
    /*类型是文件夹时*/
    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf)
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf); //传递的是地址——后续p的改变也是buf的改变
        *p++ = '/';
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            //不递归到“.”和“..”
            if (de.inum == 0 || strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0)
                continue;
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0; //让字符串结尾添加0(标准应该是\0)，表明字符串的结束。
            if (stat(buf, &st) < 0)
            {
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            /* 迭代 */
            //若是文件，则再次迭代抵达文件层面并匹配、输出。
            //若是文件夹，则迭代函数找到最底层文件。
            find(buf, name);
        }
        break;
    }
    close(fd);
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        find(argv[1], argv[2]);
        exit(0);
    }
    else
    {
        printf("Input Error!\n");
        exit(0);
    }
}
