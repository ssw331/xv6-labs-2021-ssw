#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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


#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 st_addr;  // 起始地址
  int num;
  uint64 abits;  // bitmask的地址
  if(argaddr(0, &st_addr) < 0 || argint(1, &num) < 0 || argaddr(2, &abits))
      return -1;
  struct proc* p = myproc();
  pagetable_t tp = p->pagetable;
  uint32 bitmask = 0;

  // 模仿walk()
  if(st_addr >= MAXVA)
      panic("pgaccess");
  for(int itr = 0; itr < 32; itr++, st_addr += PGSIZE) {  // 更新st_addr
      pte_t *pte = 0;
      tp = p->pagetable;
      for(int level = 2; level >= 0; level--) {  // 遍历三级页表目录得到PA
          pte = &tp[PX(level, st_addr)];  // 从虚拟地址中提取对应级别目录的索引
          // 目录级别：L2->L1->L0->PA
          if(*pte & PTE_V) {
              tp = (pagetable_t)PTE2PA(*pte);
          } // 无须alloc，所以不判定非法情况
      }
      if(*pte & PTE_V && *pte & PTE_A) {
          bitmask |= (1U << itr);  // 在bitmask中记录对应位置的访问结果
          *pte &= ~(PTE_A);  // 重置accessed标识位
      }
  }
  if(copyout(p->pagetable, abits, (char*)&bitmask, sizeof(uint64)))
      return -1;

  return 0;
}
#endif

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
