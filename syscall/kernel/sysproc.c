#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
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

  if(argint(0, &pid) < 0)
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

uint64
sys_trace(void) {
    int mask;

    // a0 保存了trace命令行的第一个参数（无trace关键字）
    // 因此需要将a0的值传给创建的新进程的proc的mask
    // 这里从a0寄存器中提取值给mask，因此不需要 1 << a0(?)
    if ((argint(0, &mask)) < 0)
        return -1;

    // 将mask给到进程
    myproc()->mask = mask;
    return 0;
}

uint64
sys_sysinfo(void) {
    struct sysinfo p;  // 记住指针没初始化时是没有空间分配的，这里是分配在内核区
    uint64 si;  // 用户域的sysinfo指针
    if(argaddr(0, &si) < 0)
        return -1;
    p.freemem = freemem();
    p.nproc = nproc();
    if(copyout(myproc()->pagetable, si, (char*) &p, sizeof(p)) < 0)
        return -1;
    return 0;
}
