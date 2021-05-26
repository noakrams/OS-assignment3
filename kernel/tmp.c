int
exec(char *path, char **argv)
{
  printf("exec\n");
  char *s, *last;
  int i, off;
  uint64 argc, sz = 0, sp, ustack[MAXARG+1], stackbase;
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pagetable_t pagetable = 0, oldpagetable;
  struct proc *p = myproc();

#if (SELECTION == SCFIFO || SELECTION == NFUA || SELECTION == LAPA)

  // back up the data
  struct page backup_RAM[MAX_PSYC_PAGES];
  struct page backup_swap[MAX_PSYC_PAGES];
  int backup_numOfMemory = 0;
  int backup_numOfPageFaults = 0;
  int backup_numOfPagedOut = 0;
  int backup_numOfSwapping = 0;
  int backup_indexSCFIFO = 0;

  if(p->pid > 2){
    // back up counters
    backup_numOfMemory = p->numOfMemoryPages;
    backup_numOfPagedOut = p->numOfPagedOut;
    backup_numOfPageFaults = p->numOfPageFaults;
    backup_numOfSwapping = p->numOfSwapping;
    backup_indexSCFIFO = p->indexSCFIFO;

    for(int i = 0; i<MAX_PSYC_PAGES; i++){
      // back up the ram array
      backup_RAM[i].vaddr = p->ramArray[i].vaddr;
      backup_RAM[i].isUnavailable = p->ramArray[i].isUnavailable;
      backup_RAM[i].pagetable = p->ramArray[i].pagetable;
      backup_RAM[i].age = p->ramArray[i].age;

      // back up the swap array
      backup_swap[i].vaddr = p->swapArray[i].vaddr;
      backup_swap[i].age = p->swapArray[i].age;
      backup_swap[i].isUnavailable = p->swapArray[i].isUnavailable;
      backup_swap[i].pagetable = p->swapArray[i].pagetable;
    }

    // zero the counters
    p->numOfMemoryPages = 0;
    p->numOfPagedOut = 0;
    p->numOfPageFaults = 0;
    p->numOfSwapping = 0;
    p->indexSCFIFO = 0;

    // zero the data
    for(int i = 0; i<MAX_PSYC_PAGES; i++){
      // zero the ram array
      p->ramArray[i].vaddr = 0xffffffff;
      p->ramArray[i].isUnavailable = 0;
      p->ramArray[i].age = 0;
      p->ramArray[i].pagetable = 0;

      // zero the swap array
      p->swapArray[i].vaddr = 0xffffffff;
      p->swapArray[i].isUnavailable = 0;
      p->swapArray[i].age = 0;
      p->swapArray[i].pagetable = 0;
    }
  }
  #endif




  begin_op();

  if((ip = namei(path)) == 0){
    end_op();
    return -1;
  }
  ilock(ip);

  // Check ELF header
  if(readi(ip, 0, (uint64)&elf, 0, sizeof(elf)) != sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pagetable = proc_pagetable(p)) == 0)
    goto bad;

  // Load program into memory.
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, 0, (uint64)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if(ph.vaddr + ph.memsz < ph.vaddr)
      goto bad;
    uint64 sz1;
    if((sz1 = uvmalloc(pagetable, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    sz = sz1;
    if(ph.vaddr % PGSIZE != 0)
      goto bad;
    if(loadseg(pagetable, ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  end_op();
  ip = 0;

  p = myproc();
  uint64 oldsz = p->sz;

  // Allocate two pages at the next page boundary.
  // Use the second as the user stack.
  sz = PGROUNDUP(sz);
  uint64 sz1;
  if((sz1 = uvmalloc(pagetable, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  sz = sz1;
  uvmclear(pagetable, sz-2*PGSIZE);
  sp = sz;
  stackbase = sp - PGSIZE;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp -= strlen(argv[argc]) + 1;
    sp -= sp % 16; // riscv sp must be 16-byte aligned
    if(sp < stackbase)
      goto bad;
    if(copyout(pagetable, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  // push the array of argv[] pointers.
  sp -= (argc+1) * sizeof(uint64);
  sp -= sp % 16;
  if(sp < stackbase)
    goto bad;
  if(copyout(pagetable, sp, (char *)ustack, (argc+1)*sizeof(uint64)) < 0)
    goto bad;

  // arguments to user main(argc, argv)
  // argc is returned via the system call return
  // value, which goes in a0.
  p->trapframe->a1 = sp;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(p->name, last, sizeof(p->name));
   
  if(p->pid>2){
    printf("change is here\n");
    removeSwapFile(p);
    createSwapFile(p);
  }  

  for(int i = 0; i<MAX_PSYC_PAGES; i++){
    if(p->ramArray[i].isUnavailable == 1){ // check all the unavaile spaces
      p->ramArray[i].pagetable = pagetable;
    }
    if(p->swapArray[i].isUnavailable == 1){ // check all the unavaile spaces
      p->swapArray[i].pagetable = pagetable;
    }
  }

  // Commit to the user image.
  oldpagetable = p->pagetable;
  p->pagetable = pagetable;
  p->sz = sz;
  p->trapframe->epc = elf.entry;  // initial program counter = main
  p->trapframe->sp = sp; // initial stack pointer
  proc_freepagetable(oldpagetable, oldsz);
  printf("I leave exec\n");
  return argc; // this ends up in a0, the first argument to main(argc, argv)

 bad:
  if(pagetable)
    proc_freepagetable(pagetable, sz);

  #if (SELECTION == SCFIFO || SELECTION == NFUA || SELECTION == LAPA)
    if(p->pid > 2){
      p->numOfMemoryPages = backup_numOfMemory;
      p->numOfPagedOut = backup_numOfPagedOut;
      p->numOfPageFaults = backup_numOfPageFaults;
      p->numOfSwapping = backup_numOfSwapping;
      p->indexSCFIFO = backup_indexSCFIFO;

      for(int i = 0; i<MAX_PSYC_PAGES; i++){
        p->ramArray[i].vaddr = backup_RAM[i].vaddr;
        p->ramArray[i].isUnavailable = backup_RAM[i].isUnavailable;
        p->ramArray[i].age = backup_RAM[i].age;
        p->ramArray[i].pagetable = backup_RAM[i].pagetable;

        p->swapArray[i].vaddr = backup_swap[i].vaddr;
        p->swapArray[i].isUnavailable = backup_swap[i].isUnavailable;
        p->swapArray[i].age = backup_swap[i].age;
        p->swapArray[i].pagetable = backup_swap[i].pagetable;
      }
    }
  #endif


  if(ip){
    iunlockput(ip);
    end_op();
  }
  return -1;
}