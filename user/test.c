#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define PAGEZS 4096

void simpleTest(){
   char* arr[32];
   arr[0] = 0;
   arr[1] = 0;
   arr[2] = 0;

    // for(int i = 0; i < 5; i++){
    //     arr[i] = sbrk(PAGEZS);
    // }
    for(int i=0; i<2; i++){
        printf("arr[%d] add : %p\n", i, arr[i]);
        arr[i][0] = 1;
    }
}

void simpleFork(){

    int pid = fork();
    int status;

    if (!pid){
        exit(0);
    }
    else{
        wait(&status);
    }
}


void SanityTest(){

    char* arr[32];
    int status;

    printf("// allocate 5 pages //\n");
    for(int i = 0; i < 5; i++){
        arr[i] = sbrk(PAGEZS);
    }

    printf("// fork //\n");
    int pid = fork();
   
    if(pid == 0){
        // printf("// iniside child //\n");
        // printf("// allocate another 15 pages for child //\n");

        // for(int i = 5; i < 20; i++){
        //     arr[i] = sbrk(PAGEZS);
        // }

        // sleep(10);

        // printf("// no pagefault in the next 3 prints //\n");

        // for(int i=16; i<19; i++){
        //     printf("arr[%d] add : %p\n", i, arr[i]);
        //     arr[i][0] = 1;
        // }

        // printf("// pagefault in the next 3 prints //\n");

        for(int i=1; i<4; i++){
            printf("arr[%d] add : %p\n", i, arr[i]);
            arr[i][0] = 1;
        }

        // printf("// child finish his job for today //\n");

        exit(0);
    }
   
    if(pid != 0){
        wait(&status);

        // printf("// iniside father //\n");
        // printf("// allocate another 5 pages for child //\n");

        // for(int i = 5; i < 10; i++){
        //     arr[i] = sbrk(PAGEZS);
        // }

        // sleep(10);

        // printf("// allocate another 10 pages for child //\n");

        // for(int i = 10; i < 20; i++){
        //     arr[i] = sbrk(PAGEZS);
        // }

        // printf("// no pagefault in the next 3 prints //\n");

        // for(int i=16; i<19; i++){
        //     printf("arr[%d] add : %p\n", i, arr[i]);
        //     arr[i][0] = 1;
        // }

        // printf("// pagefault in the next 3 prints //\n");

        // for(int i=1; i<4; i++){
        //     printf("arr[%d] add : %p\n", i, arr[i]);
        //     arr[i][0] = 1;
        // }

        // printf("// father finish his job for today //\n");

    }
}

void forkTest(){

    char* arr[32];
    int status;

    printf("// allocate 5 pages //\n");
    for(int i = 0; i < 5; i++){
        arr[i] = sbrk(PAGEZS);
    }

    printf("// fork //\n");
    int pid = fork();
   
    if(pid == 0){
        printf("// iniside child //\n");
        printf("// allocate another 5 pages for child //\n");

        for(int i = 5; i < 10; i++){
            arr[i] = sbrk(PAGEZS);
        }

        sleep(10);

        // for(int i = 10; i < 20; i++){
        //     arr[i] = sbrk(PAGEZS);
        // }

        printf("// no pagefault in the next 5 prints //\n");

        for(int i=4; i<10; i++){
            printf("arr[%d] add : %p\n", i, arr[i]);
            arr[i][0] = 1;
        }

        // printf("// pagefault in the next 5 prints //\n");

        // for(int i=14; i<19; i++){
        //     printf("arr[%d] add : %p\n", i, arr[i]);
        //     arr[i][0] = 1;
        // }

        printf("// child finish his job for today //\n");

        exit(0);
    }
   
    if(pid != 0){
        wait(&status);
    }
}

void pageFaultTest(){

    char* arr[32];

    printf("// allocate 20 pages //\n");
    for(int i = 0; i < 20; i++){
        arr[i] = sbrk(PAGEZS);
    }

    printf("// no pagefault in the next 3 prints //\n");

    for(int i=16; i<19; i++){
        printf("arr[%d] add : %p\n", i, arr[i]);
        arr[i][0] = 1;
    }
    
    printf("// pagefault in the next 3 prints //\n");

    for(int i=0; i<3; i++){
        printf("arr[%d] add : %p\n", i, arr[i]);
        arr[i][0] = 1;
    }
}

int
main(int argc, char *argv[])
{
    int test_number = 1;
    printf("starting test\n");

    printf("-----test no %d-----\n", test_number);
    simpleTest();
    test_number++;

    // printf("-----test no %d-----\n", test_number);
    // simpleFork();
    // test_number++;

    // printf("-----test no %d-----\n", test_number);
    // SanityTest();
    // test_number++;

    // printf("\n-----test no %d-----\n", test_number);
    // forkTest();
    // test_number++;

    // printf("\n-----test no %d-----\n", test_number);
    // pageFaultTest();
    // test_number++;

    printf("\nfinished test successfully\n");
    exit(0);

}