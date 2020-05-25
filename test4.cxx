#include "hw2_test.h"
#include <unistd.h>
#include <sys/wait.h>

int main() {
    double vruntime1 = 0.0 , vruntime2 = 0.0;
    double timeout = 2.0;
    double check = timeout / 2;
    double ratio = 1024 / (1024.0 + 335.0);
    Stopwatch stopwatch1;
    Stopwatch stopwatch2;
    pid_t pid = fork();
    if(pid == 0){
        vruntime1 = get_vruntime();
        while (stopwatch1.Read() < timeout);
        vruntime1 = get_vruntime() - vruntime1;
        AssertRelativeError(check, vruntime1, 0.1);
        stopwatch1.Reset();
        exit(0);
    }
    if(pid > 0){
        vruntime2 = get_vruntime();
        while (stopwatch2.Read() < timeout);
        vruntime2 = get_vruntime() - vruntime2;
        AssertRelativeError(check, vruntime2, 0.1);
        stopwatch2.Reset();
        wait(nullptr);

        pid = fork();
        if(pid == 0){
            if (nice(5) == -1)
                return -1;
            stopwatch1.Reset();
            vruntime1 = get_vruntime();
            while (stopwatch1.Read() < timeout);
            vruntime1 = get_vruntime() - vruntime1;
            AssertRelativeError(ratio*timeout, vruntime1, 0.1);
            stopwatch1.Reset();
            exit(0);
        }
        if(pid > 0){
            vruntime2 = get_vruntime();
            while (stopwatch2.Read() < timeout);
            vruntime2 = get_vruntime() - vruntime2;
            AssertRelativeError(ratio*timeout, vruntime2, 0.1);
            stopwatch2.Reset();
            wait(nullptr);
            cout << "=====SUCCESS=====" << endl;
            return 0;
        }
    }
}

