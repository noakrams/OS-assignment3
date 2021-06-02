#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

struct objectTest {
    int arr[1024];
};

void forkTest11(){
    int pid = 0;
    pid = fork();
   
    if(pid == 0){
        printf("&&&i'm child %d, going to do my first malloc\n", getpid());
        // sleep(10);

        struct objectTest *t = malloc(4096);
        printf("&&&i after 1 malloc\n");
        // sleep(10);
        t->arr[0] = 24;
        t->arr[511] = 500;
        printf("&&&i after changes\n");
        // sleep(10);
        for(int i = 0; i < 10; i++){
            malloc(4096);
            // sleep(1);
            printf("&&&i after %d mallocs\n", i+2);

        }
        printf("&&&i after all mallocs\n");

        printf("SON: \n");
        printf("%d\n", t->arr[0]);
        printf("%d\n", t->arr[511]);
    }
   

    if(pid != 0){
        wait(&pid);
        printf("son finished, dad continues\n");
        struct objectTest *t = malloc(4096);
        sbrk(10*4096);
        t->arr[0] = 1;
        printf("FATHER: %d\n", t->arr[0]);
    }
   
    exit(0);
}

int main (){
    forkTest11();
    exit(0);
}