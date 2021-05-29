#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

// count the number of bits in a num
int
countBits(int num){
  int count = 0;
  int i = 0;

  while(i<32){
    count += num & 1;
    num >>= 1;
    i++;
  }

  return count;
}


int
pageToSwapFile(){

  #ifdef NFUA

  struct proc *p = myproc();
  printf("NFUA with pid %d\n", p->pid);

  struct page_md *pagemd;
  int swapfile_offset;

  // finding the lowest counter

  int min_value= 2147483647; //max value of int
  int min_page=-1;
  for(int i = 0; i< MAX_TOTAL_PAGES; i++){
    pagemd = &p->total_pages[i];
    if(pagemd->stat == MEMORY){
      if(pagemd->counter < min_value){
        min_value = pagemd->counter;
        min_page = i;
      }
    }
  }

  if(min_page == -1)
    return 0;

  if((swapfile_offset = find_free_offset()) < 0)
    return 0;

    // move the page to file

  pagemd = &p->total_pages[min_page];
  pagemd -> offset = swapfile_offset * PGSIZE;
  pagemd -> counter = 0;
  pagemd -> stat = FILE;
  pagemd -> ctime = -1;
  
  pte_t* pteToRemove = walk(p->pagetable, (uint64)pagemd->va, 0);
  if(writeToSwapFile(p, (char*)(PTE2PA(*pteToRemove)), pagemd -> offset, PGSIZE) == 0){
    panic("return 0 from writeToSwapFile\n");
    return 0;
  }

  p->file_pages[swapfile_offset] = 1;
  
  *pteToRemove |= PTE_PG; // in disk
  *pteToRemove &= ~PTE_V; // not valid

  p->swapPages ++;
  p->ramPages --;
  kfree((void*)(PTE2PA(*pteToRemove)));
  return 1;

  #else

  #ifdef LAPA

  struct proc *p = myproc();
  printf("LAPA with pid %d\n", p->pid);

  struct page_md *pagemd;
  int swapfile_offset;

  // finding the smallest number of "1" counter
  
  int min_number_of_1= 33;     //max number of 1 in int
  int min_value= 2147483647;  //max value of int
  int min_page=-1;
  for(int i = 0; i< MAX_TOTAL_PAGES; i++){
    pagemd = &p->total_pages[i];
    if(pagemd->stat == MEMORY){
      int count = countBits(pagemd->counter);
      if(count < min_number_of_1){
        min_number_of_1 = count;
        min_value = pagemd->counter;
        min_page = i;
      }
      else if (count == min_number_of_1){
        if(pagemd->counter < min_value){
          min_value = pagemd->counter;
          min_page = i;
        }
      }
    }
  }
  if(min_page == -1)
    return 0;

  if((swapfile_offset = find_free_offset()) < 0)
    return 0;

    // move the page to file

  pagemd = &p->total_pages[min_page];
  pagemd -> counter = 0xFFFFFFFF;
  pagemd -> stat = FILE;
  pagemd -> offset = swapfile_offset * PGSIZE;
  pagemd -> ctime = -1;

  pte_t* pteToRemove = walk(p->pagetable, (uint64)pagemd->va, 0);
  if(writeToSwapFile(p, (char*) (PTE2PA(*pteToRemove)), pagemd -> offset, PGSIZE) == 0){
    panic("return 0 from writeToSwapFile\n");
    return 0;
  }

  p->file_pages[swapfile_offset] = 1;

  *pteToRemove |= PTE_PG; // in disk
  *pteToRemove &= ~PTE_V; // not valid

  p->swapPages ++;
  p->ramPages --;
  kfree((void*)(PTE2PA(*pteToRemove)));
  return 1;

  #else

  #ifdef SCFIFO

  struct proc *p = myproc();
  printf("SCFIFO with pid %d\n", p->pid);

  struct page_md *pagemd;
  int ctime;
  int place;
  int swapfile_offset;

  while(1){
    ctime = -1;
    place = -1;
    for(int i = 0; i< MAX_TOTAL_PAGES; i++){
      pagemd = &p->total_pages[i];
      if(pagemd->stat == MEMORY){
        if (ctime == -1 || pagemd->ctime < ctime){
          ctime = pagemd->ctime;
          place = i;
        }
      }
    }
    pagemd = &p->total_pages[place];
    pte_t *pte = walk(p->pagetable, pagemd->va, 1);
    if(*pte & PTE_A){
      pagemd->ctime = ticks;  //update time
      *pte &= ~PTE_A;         //clear access flag
    }
    else goto found;
  }

  found:

  // move the page to file

  if((swapfile_offset = find_free_offset()) < 0)
    return 0;

  pagemd -> counter = 0;
  pagemd -> stat = FILE;
  pagemd -> offset = swapfile_offset * PGSIZE;
  pagemd -> ctime = -1;

  pte_t* pteToRemove = walk(p->pagetable, (uint64)pagemd->va, 0);

  if(writeToSwapFile(p, (char*) (PTE2PA(*pteToRemove)), pagemd -> offset, PGSIZE) == 0)
    return 0;

  *pteToRemove |= PTE_PG; // in disk
  *pteToRemove &= ~PTE_V; // not valid

  p->file_pages[swapfile_offset] = 1;
  p->swapPages ++;
  p->ramPages --;
  
  kfree((void*)(PTE2PA(*pteToRemove)));
  return 1;

  #endif
  #endif
  #endif

  return 1;
}

void
trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void
trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void
usertrap(void)
{
  int which_dev = 0;

  if((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();
  
  // save user program counter.
  p->trapframe->epc = r_sepc();
  
  if(r_scause() == 8){
    // system call

    if(p->killed)
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sstatus &c registers,
    // so don't enable until done with those registers.
    intr_on();

    syscall();
  } else if((which_dev = devintr()) != 0){
    // ok
  } else {

    #ifndef NONE
    uint64 scause = r_scause();
    uint64 stval = r_stval();

    uint64 va = PGROUNDDOWN(stval);
    pte_t *pte = walk(p->pagetable, va, 1);

    // If segmentation fault is 13 or 15 and page is in swap file 
    if(p->pid > 2 && (scause == 12 || scause == 13 || scause == 15) && PAGEDOUT(*pte)){

      struct page_md* currPage;

      // Try and find the page with the requested va
      for(int i = 0 ; i < MAX_TOTAL_PAGES ; i++){
        currPage = &p->total_pages[i];
        if (currPage->va == va && currPage->stat == FILE) {
            goto found;
        }
      }
      printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
      printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
      p->killed = 1; 

      found:

      printf("Page fault, read page from swapfile\n");

      // Read data from swap file to the virtual address of the page
      if(readFromSwapFile(p, (char*)(PTE2PA(*pte)), currPage->offset * PGSIZE, PGSIZE) == -1)
        panic("can't read from swap file");

      if (p->ramPages == MAX_PSYC_PAGES){
        p->file_pages[currPage->offset] = 0;
        pageToSwapFile();
      }

      // refresh TLB
      // sfence_vma();
      *pte &= ~PTE_PG; // Turn off swap bit
      *pte |= PTE_V; // Turn on valid bit

      // Update process and current page
      currPage->offset = -1;
      currPage->stat = MEMORY;
      currPage->ctime = ticks;
      p->file_pages[currPage->offset/PGSIZE] = 0;
      p->ramPages++;
      p->swapPages--;
    }

    else{
      printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
      printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
      p->killed = 1;
    }

    #else
      printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
      printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
      p->killed = 1;
    #endif
  }

  if(p->killed)
    exit(-1);

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2)
    yield();

  usertrapret();
}

//
// return to user space
//
void
usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to trampoline.S
  w_stvec(TRAMPOLINE + (uservec - trampoline));

  // set up trapframe values that uservec will need when
  // the process next re-enters the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp();         // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.
  
  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to trampoline.S at the top of memory, which 
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 fn = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64,uint64))fn)(TRAPFRAME, satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void 
kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();
  
  if((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if(intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if((which_dev = devintr()) == 0){
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

  // give up the CPU if this is a timer interrupt.
  if(which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();

  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void
clockintr()
{
  acquire(&tickslock);
  ticks++;
  wakeup(&ticks);
  release(&tickslock);
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int
devintr()
{
  uint64 scause = r_scause();

  if((scause & 0x8000000000000000L) &&
     (scause & 0xff) == 9){
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if(irq == UART0_IRQ){
      uartintr();
    } else if(irq == VIRTIO0_IRQ){
      virtio_disk_intr();
    } else if(irq){
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if(irq)
      plic_complete(irq);

    return 1;
  } else if(scause == 0x8000000000000001L){
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if(cpuid() == 0){
      clockintr();
    }
    
    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  } else {
    return 0;
  }
}

