// #include "user.h"
// #include "types.h"

#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"
#include "kernel/syscall.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"

// #define NONE 0
// #define NFUA 1
// #define LAPA 2

struct objectTest {
    int arr[1024];
};


int test_no = 0;

int very_simple(int pid){
    if(pid == 0){
        printf("rest...\n");
        sleep(20);
        printf("exit...\n");
        exit(0);
    }
    if(pid > 0){
        printf("Parent waiting on test %d\n", test_no);
        wait(0);
        printf("Done!\n");
        return 1;
    }
    return 0;
}


int simple(int pid){
    if(pid == 0){
        printf("try alloc, access, free...\n");
        char* trash = malloc(3);
        trash[1] = 't' ;
        if(trash[1] != 't'){
            printf("Wrong data in simple! %c\n", trash[1]);
        }
        free(trash);
        printf("Done malloc dealloc, exiting...\n");
        exit(0);
    }
    if(pid > 0){
        printf("Parent waiting on test %d\n", test_no);
        wait(0);
        printf("Done! malloc dealloc\n");
        return 1;
    }
    return 0;
}


int test_paging(int pid, int pages){
    if(pid == 0){
        int size = pages;
        char* pp[size];
        for(int i = 0 ; i < size ; i++){
            printf("Call %d for sbrk\n", i);
            pp[i] = sbrk(PGSIZE-1);
            *pp[i] = '0' +(char) i ;
        }
        printf("try accessing this data...\n");

        if(size > 4 && *pp[4] != '4'){
            printf("Wrong data in pp[4]! %c\n", *pp[4]);
        }

        if(*pp[0] != '0'){
            printf("Wrong data in pp[0]! %c\n", *pp[0]);
        }

        if(*pp[size-1] != '0' +(char) size-1 ){
            printf("Wrong data in pp[size-1]! %c\n", *pp[size-1]);
        }

        printf("Done allocing, exiting...\n");
        exit(0);

    }

    if(pid >  0){
        printf("Parent waiting on test %d\n", test_no);
        wait(0);
        printf("Done! alloce'd some pages\n");
        return 1;
    }
    return 0;
}


void forkTest(){
    int pid = 0;
    pid = fork();
   
    if(pid == 0){
        struct objectTest *t = malloc(4096);
        t->arr[0] = 24;
        t->arr[511] = 500;
        for(int i = 0; i < 10; i++){
            malloc(4096);
        }

        printf("SON: \n");
        printf("%d\n", t->arr[0]);
        printf("%d\n", t->arr[511]);
    }
    if(pid != 0){
        wait(&pid);

    // if(pid != 0){
        struct objectTest *t = malloc(4096);
        sbrk(20*4096);
        t->arr[0] = 1;
        printf("FATHER: %d\n", t->arr[0]);
    }
   
    exit(0);
}




int main(int argc, char *argv[]){

    printf("--------- START TESTING! ---------\n");


    printf("------- test%d -------\n", test_no);
    very_simple(fork());
    test_no++;

    printf("------- test%d -------\n", test_no);
    simple(fork());
    test_no++;

    printf("------- test%d -------\n", test_no);
    test_paging(fork(),2);
    test_no++;

    printf("------- test%d -------\n", test_no);
    test_paging(fork(),6);
    test_no++;

    printf("------- test%d -------\n", test_no);
    test_paging(fork(),13);
    test_no++;

    printf("------- test%d -------\n", test_no);
    forkTest();
    test_no++;

    // TODO: ADD MORE TESTS!!


    printf("--------- DONE  TESTING! ---------\n");
    return 0;
}