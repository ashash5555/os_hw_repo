#include<stdio.h>
#include<stdlib.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
// #include <linux/user.h>
#include<sys/user.h>
#include<sys/reg.h>

int main() {
    printf("calling fork\n");
    pid_t pid = fork();
    if (pid > 0) {
        printf("father\n");
        int wait_status;
        wait(&wait_status);

        while (WIFSTOPPED(wait_status)) {
            /// this struct stores all user registers
            struct user_regs_struct regs;   
            /// this call will send signal right before child executes syscall
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            /// wait for child to execute syscall
            wait(&wait_status);
            /// this will send a signal to the child to run the syscall
            ptrace(PTRACE_SYSCALL, pid, NULL, NULL);
            /// wait for him to finish
            wait(&wait_status);  

            ptrace(PTRACE_GETREGS, pid, NULL, &regs);
            printf("DBG: syscall number: %lld. syscall returned: %lld\n", regs.orig_rax, regs.rax);

            /// send signal to child to continue
            ptrace(PTRACE_CONT, pid, 0, 0);
        }

    } else if (pid == 0) {
        printf("child\n");
        if (ptrace(PTRACE_TRACEME, 0, NULL, NULL) < 0) {
            perror("ptrace");
            exit(1);
        }
        size_t bytes = 0;
        /// get 'x' from user. (we assume we get 1 positive number)
        scanf("%lu", &bytes);
        /// allocate block of size 'x'
        void* ptr = malloc(bytes);
        exit(0);
    } else {
        perror("fork");
        exit(1);
    }
    
    return 0;
}

// void my_awesome_debbuger(pid_t child_pid) {
//     int wait_status;
//     wait(&wait_status);

//     while (WIFSTOPPED(wait_status)) {
//         /// this struct stores all user registers
//         struct user_regs_struct regs;   
//         /// this call will send signal right before child executes syscall
//         ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
//         /// wait for child to execute syscall
//         wait(&wait_status);
//         /// this will send a signal to the child to run the syscall
//         ptrace(PTRACE_SYSCALL, child_pid, NULL, NULL);
//         /// wait for him to finish
//         wait(&wait_status);  

//         ptrace(PTRACE_GETREGS, child_pid, NULL, &regs);
//         printf("DBG: syscall number: %lld. syscall returned: %lld\n", regs.orig_rax, regs.rax);

//         /// send signal to child to continue
//         ptrace(PTRACE_CONT, child_pid, 0, 0);
//     }
// }