#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>

using namespace std;

void ctrlZHandler(int sig_num) {
	// TODO: Add your implementation
	cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* job = smash.getCurrentJob();
    int res;
    if (job) {
        pid_t pid = job->getJobPID();
        res = kill(pid, SIGSTOP);
        if (res != 0) {
            perror("smash error: kill failed");
            return;
        }
        smash.addSleepingJob(job);
    }
}

void ctrlCHandler(int sig_num) {
  // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    JobsList::JobEntry* job = smash.getCurrentJob();
    int res;
    if (job) {
        pid_t pid = job->getJobPID();
        res = kill(pid, SIGKILL);
        if (res != 0) {
            perror("smash error: kill failed");
            return;
        }
        int jobID = job->getJobID();
        bool jobInList = smash.jobInList(jobID);
        bool deleteEntry = (jobID != -1 && !jobInList);
        smash.clearCurrentJob(deleteEntry);
        cout << "smash: process " << pid << " was killed" << endl;
    }
}

void alarmHandler(int sig_num, siginfo_t* siginfo, void* context) {
  // TODO: Add your implementation
    cout << "smash: got an alarm" << endl;
    SmallShell& smash = SmallShell::getInstance();
    TimeoutEntry* entry = smash.getEntryToKill();
    pid_t pid = entry->getPID();
    pid_t smashPID = getpid();
    int res = -1;
    if (pid != smashPID) {
        res = kill(pid, SIGKILL);
        if (res != 0) {
            perror("smash error: kill failed");
            return;
        }
    }
    else {
        /// if we got here there is a problem
        res = kill(pid, SIGSTOP);
        if (res != 0) {
            perror("smash error: kill failed");
            return;
        }
    }
    string cmd = entry->getCmd();
    cout << "smash: " << cmd << " timed out!" << endl;
    if (!smash.isTimedEmpty()) {
        int alarm_duration = smash.getMinTimeLeft();
        alarm(alarm_duration);
    }
}

