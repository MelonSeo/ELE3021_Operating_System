#include "kernel/types.h"
#include "user.h"
#include "kernel/riscv.h"
#include "thread.h"
#include "stddef.h"


int thread_create(void(*start_routine)(void *, void *), void *arg1, void *arg2) {
    void *stack = malloc(PGSIZE);
    if(stack == NULL)
        return -1;

    int pid = clone(start_routine, arg1, arg2, stack);
    if (pid<0) {
        free(stack);
        return -1;
    }
    return pid;
}

int thread_join() {
    void* stack = 0;
    int pid = join(&stack);
    if (pid < 0)
        return -1;
    if (stack)
        free(stack);
    return pid;
}