#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"


void hi(int num){
    printf("%d Hi!\n", num);
}

void hello(int num){
    printf("%d Hello!\n", num);
}

void hola(int num){
    printf("%d Hola!\n", num);
}

// print a lot of things to screen - call 3 different functions 
// the process will need to swap pages
void greet(){
    for(int i = 0; i < 1000; i ++){
        hi(i);
        hello(i);
        hola(i);
    }
}

// this test does a lot of calculations with a malloced array
// each calculation need the previous ones
// need to read and write to the array
void calc(){
    int *arr = (int *)malloc(2000 * sizeof(int));
    int i, j;
    uint counter = 0;

    for(i = 0; i < 2000; i++){
        *(arr + i) = i;
    }
    for(i = 0; i < 2000; i++){
        for(j = 0; j <= i; j ++){
            counter += *(arr + j);
        }
        printf("%d: counter is %d\n", i, counter);
    }
    printf("final counter is %d\n",  counter);
} 

// same test as above but with fork
void fork_calc(){
    int pid = fork();
    if(pid == 0){
        calc();
        printf("PID %d\n", getpid());
    }
    else{
        int pid2 = fork();
        if(pid2 == 0){
            calc();
            printf("PID %d\n", getpid());
        }
        else{
            calc();
            printf("PID %d\n", getpid());
            wait(0);
        }
    }
    exit(0);
}

// malloc array and run over rows-columns
void matrix_raws(){
    int i, j;
    int counter = 0;
    int *arr = (int *)malloc(100 * 100 * sizeof(int));
    
    for (i = 0; i < 100; i++) {
        for (j = 0; j < 100; j++) {
            *(arr + i*100 + j) = counter;
        }
    } 
    free(arr);
}

// malloc array and run over columns-rows
void matrix_columns(){
    int i, j;
    int *arr = (int *)malloc(100 * 100 * sizeof(int));
    for (j = 0; j < 100; j++) {
        for (i = 0; i < 100; i++) {
            *(arr + i*100 + j) = 0;
        }
    }
    free(arr);
}


// this test creates a lot of process
// each process malloc an array of size 100*100*sizeof(int)
// works faster with NONE 
void check_fork(){
    for(int i = 0; i < 5; i ++){
        int j = fork();
        if(j == 0){
            matrix_raws();
            printf("S PID %d\n", getpid());
        }
        else{
            matrix_columns();
            printf("F PID %d\n", getpid());
            wait(0);
        }
    }
    exit(0);
}

// test sbrk with different sizes - should cause 1 page fault
void test_sbrk(){
  int pid = fork();
  char *a; 

  if(pid == 0){
    int sz = (uint64) sbrk(0);
    a = sbrk(-sz);
    printf("did sbrk 1 %p\n", a);
    // user page fault - all code was deleted
    exit(0);
  }
  else
    wait(0);

  pid = fork();
  if(pid == 0){
    int sz = (uint64) sbrk(0);
    printf("did sbrk 2 %p\n", sz);
    // delete half of size
    a = sbrk(-(sz - 1500));
    exit(0);
  }
  wait(0);

  pid = fork();
  if(pid == 0){
    int sz = (uint64) sbrk(0);
    sbrk((10*PGSIZE + 2048) - sz);
    // delete just a bit
    a = sbrk(-10);
    printf("did sbrk 3 %p\n", a);
    exit(0);
  }
  else{
    wait(0);
    printf("finised\n");
    exit(0);
  }
}

void check_fork_PGSIZE(){
    int f = fork();

    if(f == 0){
        char* arr = (char*)(malloc(sizeof(char)*PGSIZE*12));
        for(int i=PGSIZE*0; i<PGSIZE*1; i++){
            arr[i]='a';
        }
        printf("Create child #1\n");
        int f2 = fork();
        if(f2 == 0){
            for(int i=PGSIZE*1; i<PGSIZE*2; i++){
                arr[i]='b';
            }
            exit(0);
        }else{
            wait(0);
        }
        printf("Create child #2\n");
        int f3 = fork();
        if(f3 == 0){
            for(int i=PGSIZE*2; i<PGSIZE*3; i++){
                arr[i]='c';
            }
            exit(0);
        }else{
            wait(0);
        }
        printf("Create child #3\n");
        exit(0);
    }else{
        wait(0);
    }
}


int main(){
    // check_fork();
    // check_fork_PGSIZE();
    // greet();
    // calc();
    // test_sbrk();
    fork_calc();
    exit(0);
}
