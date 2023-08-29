#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
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
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
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


  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
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
    pte_t *pte;
    int npages;
    uint64 p_abits, buf;
    unsigned int foo = 0;
    pagetable_t pagetable = myproc()->pagetable;

    argaddr( 0, &buf );
    argint( 1, &npages );
    argaddr( 2, &p_abits );

    for( int i = 0; i < npages; ++i ) {
        if( (pte = walk(pagetable, buf + i * PGSIZE, 0)) == 0 ) {
            panic("sys_pgacess: walk");
            return -1;
        }

        if( *pte & PTE_A ) {
            foo |= (1 << i);
            *pte &= (~PTE_A);
        }
    }

    if( copyout(pagetable, p_abits, (char *)&foo, sizeof(unsigned int)) < 0 ) {
        panic("sys_pgacess: copyout");
        return -1;
    }
    return 0;
}
#endif

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
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
