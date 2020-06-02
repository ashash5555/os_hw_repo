#include "hw2_test.h"
#include<sys/wait.h>
#include<sys/resource.h>

int main() {
    double timeout = 5.0;
    double vtime0 = get_vruntime();
    Stopwatch stopwatch;
    while (stopwatch.Read() < timeout); // spin
    double measured_vruntime = get_vruntime() - vtime0;
    AssertRelativeError(timeout, measured_vruntime);
    cout << "===== SUCCESS =====" << endl;
    return 0;
}





// int main() {
//     double timeout = 5.0;
//     double vtime = 0;
//     double measured_vruntime_first = 0;
//     double measured_vruntime_second = 0;
    
//     int which = PRIO_PGRP;
//     id_t pid = getpid();
//     int priority = 0;
//     setpriority(which, pid, priority);

//     int firstPipe[2];
//     int secondPipe[2];
//     pipe(firstPipe);
//     pipe(secondPipe);

//     Stopwatch stopwatch;

//     pid_t first = fork();
//         if (first < 0) {
//             cout << "Error while forking" << endl;
//             return -1;
//         }
//         else if (first == 0) {  /// First child process
//             vtime = get_vruntime();
//             while (stopwatch.Read() < timeout); // spin
//             measured_vruntime_first = get_vruntime() - vtime;
//             close(firstPipe[0]);
//             dup2(firstPipe[1], 1);
//             close(firstPipe[1]);
//             write(1, &measured_vruntime_first, sizeof(measured_vruntime_first));
//             exit(0);
//         }
//         else {  /// Father
//             pid_t second = fork();
//             if (first < 0) {
//                 cout << "Error while forking" << endl;
//                 return -1;
//             }
//             else if (second == 0) {     /// Second child process
//                 vtime = get_vruntime();
//                 while (stopwatch.Read() < timeout); // spin
//                 measured_vruntime_second = get_vruntime() - vtime;
//                 close(secondPipe[0]);
//                 dup2(secondPipe[1], 1);
//                 close(secondPipe[1]);
//                 write(1, &measured_vruntime_second, sizeof(measured_vruntime_second));
//                 exit(0);
//             }
//             else {  /// Father
//                 close(firstPipe[1]);
//                 close(secondPipe[1]);
//                 read(firstPipe[0], &measured_vruntime_first, sizeof(measured_vruntime_first));
//                 close(firstPipe[0]);
//                 read(secondPipe[0], &measured_vruntime_second, sizeof(measured_vruntime_second));
//                 close(secondPipe[0]);
//                 while (wait(NULL) > 0){}
//                 AssertRelativeError(measured_vruntime_first, measured_vruntime_second);
//                 cout << "===== SUCCESS =====" << endl;
//             }
//         }

//     return 0;
// }