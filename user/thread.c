/**
 * @brief:   Test the thread implementation
 * @date:    2023/11/5
 * @details: https://users.cs.utah.edu/~aburtsev/238P/2019spring/hw/hw4-threads.html
*/

#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/proc.h"

struct balance {
    char name[32];
    int amount;
};

// struct thread_spinlock
// {
//    uint locked; 
// };
// struct thread_spinlock balance_lock;

// int stack1[4096] __attribute__ ((aligned (4096)));
// int stack2[4096] __attribute__ ((aligned (4096)));

volatile int total_balance = 0;

volatile unsigned int delay (unsigned int d) {
   unsigned int i; 
   for (i = 0; i < d; i++) {
       __asm volatile( "nop" ::: );
   }

   return i;   
}

// void thread_spin_init(struct thread_spinlock *lk);
// void thread_spin_lock(struct thread_spinlock *lk);
// void thread_spin_unlock(struct thread_spinlock *lk);

void do_work(void *arg){
    int i; 
    int old;
   
    struct balance *b = (struct balance*) arg; 
    printf("Starting do_work: s:%s\n", b->name);

    for (i = 0; i < b->amount; i++) { 
        //  thread_spin_lock(&balance_lock);
         old = total_balance;
         delay(100000);
         total_balance = old + 1;
        //  thread_spin_unlock(&balance_lock);
    }
  
    printf("Done s:%s\n", b->name);

    thread_exit();
    return;
}

int main(int argc, char *argv[]) {

   // thread_spin_init(&balance_lock);
  
  struct balance b1 = {"b1", 3200};
  struct balance b2 = {"b2", 2800};

  void *s1, *s2;
  int t1, t2, r1, r2;

  s1 = malloc(4096);
  s2 = malloc(4096);

  t1 = thread_create(do_work, (void*)&b1, s1);
  t2 = thread_create(do_work, (void*)&b2, s2); 

  r1 = thread_join();
  r2 = thread_join();

  free(s1);
  free(s2);
  
  printf("Threads finished: (%d):%d, (%d):%d, shared balance:%d\n", 
      t1, r1, t2, r2, total_balance);

  exit(0);
}