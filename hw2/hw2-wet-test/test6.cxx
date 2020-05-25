#include "hw2_test.h"
#include <unistd.h>
#include <sys/wait.h>

int main(){
    double before, after;
    double timeout = 2.0;
    const double s_to_ns = 1e+9;
    Stopwatch stopwatch1;
    Stopwatch stopwatch2;
    before = get_vruntime();
    long delta_ns = static_cast<long>((-50) * s_to_ns);
    long r = syscall(335, (long)delta_ns);
    assert(r == -1);
    after = get_vruntime();
    AssertRelativeError(after, before);
    pid_t pid = fork();
    if(pid == 0){
        before = get_vruntime();
        increment_vruntime(50);
        after = get_vruntime();
        AssertRelativeError(after, before + 50);
        stopwatch1.Reset();
        before = get_vruntime();
        while (stopwatch2.Read() < timeout);
        after = get_vruntime();
        AssertRelativeError(before, after);
        exit(0);
    }
    if(pid > 0){
        before = get_vruntime();
        while (stopwatch2.Read() < timeout);
        after = get_vruntime();
        AssertRelativeError(after, before + timeout);
        stopwatch1.Reset();
        stopwatch2.Reset();
        wait(nullptr);

        pid = fork();
        if(pid == 0){
            before = get_vruntime();
            while (stopwatch2.Read() < timeout);
            after = get_vruntime();
            AssertRelativeError(after, before + timeout);
            exit(0);
        }
        if(pid > 0){
            before = get_vruntime();
            increment_vruntime(50);
            after = get_vruntime();
            AssertRelativeError(after, before + 50);
            before = get_vruntime();
            while (stopwatch2.Read() < timeout);
            after = get_vruntime();
            AssertRelativeError(before, after);
            stopwatch1.Reset();
            stopwatch2.Reset();
            wait(nullptr);
            cout << "=====SUCCESS=====" << endl;
            return 0;
        }
    }

}