#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void long_task() {
  unsigned long dummy = 0;
  for (volatile int j = 0; j < 100000000; j++) {
    dummy += j % 7;
  }
}

int main() {
  fcfsmode(); // 처음에 FCFS 모드

  printf("Main (pid=%d): FCFS mode started.\n", getpid());

  int pid1 = fork();
  if (pid1 == 0) {
    printf("Child1 (pid=%d): start\n", getpid());
    sleep(5); 
    printf("Child1 (pid=%d): switching to MLFQ mode\n", getpid());
    mlfqmode(); 
    exit(0);
  }

  wait(0); 
  printf("Main (pid=%d): MLFQ mode should be active now.\n", getpid());

  int pid2 = fork();
  if (pid2 == 0) {
    for(int i=0 ; i<20 ; i++) {
      printf("Child2 (pid=%d): start at level %d\n", getpid(), getlev());
      long_task();
    }
    exit(0);
  }

  int pid3 = fork();
  if (pid3 == 0) {
    for(int i=0 ; i<20 ; i++) {
      printf("Child3 (pid=%d): start at level %d\n", getpid(), getlev());
      long_task();
    }
    exit(0);
  }

  // setpriority 테스트
  sleep(50) ;
  printf("Main (pid=%d): setting priority of pid %d to 0\n", getpid(), pid3);
  if (setpriority(pid3, 3) < 0) {
    printf("Main (pid=%d): setpriority failed!\n", getpid());
  } else {
    printf("Main (pid=%d): setpriority success!\n", getpid());
  }

  wait(0); // child2
  wait(0); // child3
  wait(0); // child4

  printf("Main (pid=%d): Switching back to FCFS mode\n", getpid());
  fcfsmode(); 

  int pid5 = fork();
  if (pid5 == 0) {
    printf("Child5 (pid=%d): start\n", getpid());
    long_task(7000000);
    printf("Child5 (pid=%d): end\n", getpid());
    exit(0);
  }

  wait(0); // child5

  printf("Main (pid=%d): Test complete.\n", getpid());
  exit(0);
}