#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if (argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if (argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

/**系统调用信息打印功能**/
uint64
sys_trace(void)
{
  int n;
  struct proc *p = myproc();

  if (argint(0, &n) < 0)
    return -1;
  else
  {
    p->mask = n;
    return 0;
  }
}

/**收集xv6运行信息**/
uint64
sys_sysinfo(void)
{
  uint64 st;
  struct proc *p = myproc();
  struct
  {
    uint64 freemem; //当前剩余的内存字节数
    uint64 nproc;   //状态为UNUSED的进程个数
    uint64 freefd;  //当前进程可用文件描述符的数量，即尚未使用的文件描述符数量
  } sysinfo;

  sysinfo.freemem = kalloc_info();        
  sysinfo.nproc = nproc_info();       
  sysinfo.freefd = FreeDescriptor_info(); 

  //获取用户态结构体地址（不可在内核态更改，只能通过copyout传）
  if (argaddr(0, &st) < 0)
    return -1;
  else
  {
    if (copyout(p->pagetable, st, (char *)&sysinfo, sizeof(sysinfo)) < 0)
      return -1;
    else
      return 0;
  }
}
