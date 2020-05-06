#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"
#include <unistd.h>
//#include <typeinfo>



using namespace std;

void ctrlZHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-Z" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t smashPID = smash.getSmashPid();
    JobsList::JobEntry* job = smash.getCurrentJob();
    int res = -1;
    if (job) {
        pid_t pid = job->getJobPID();
        if (pid == smashPID) return;

        Command* cmd = smash.CreateCommand(job->getJobCmd().c_str());
        bool isTimed = (dynamic_cast<TimeoutCommand*>(cmd) != nullptr);
        //bool isTimed = (typeid(*cmd) == typeid(TimeoutCommand));
        if (isTimed) {
            cmd = smash.CreateCommand(static_cast<TimeoutCommand&>(*cmd).getCmdToRun());
        }
        bool isPipe = (dynamic_cast<PipeCommand*>(cmd) != nullptr);
        //bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
        bool isRedir = (dynamic_cast<PipeCommand*>(cmd) != nullptr);
        //bool isRedir = (typeid(*cmd) == typeid(RedirectionCommand));
        delete cmd;
        if (isPipe || isRedir) {
            res = killpg(pid, SIGSTOP);
        }
        else {
            res = kill(pid, SIGSTOP);
        }
        if (res != 0) {
            perror("smash error: kill failed");
            return;
        }
        cout << "smash: process " << pid <<  " was stopped" << endl;
        smash.addSleepingJob(job);
    }
}

void ctrlCHandler(int sig_num) {
    // TODO: Add your implementation
    cout << "smash: got ctrl-C" << endl;
    SmallShell& smash = SmallShell::getInstance();
    pid_t smashPID = smash.getSmashPid();
    JobsList::JobEntry* job = smash.getCurrentJob();
    int res = -1;
    if (job) {
        pid_t pid = job->getJobPID();
        if (pid == smashPID) return;

        Command* cmd = smash.CreateCommand(job->getJobCmd().c_str());
        bool isTimed = (dynamic_cast<TimeoutCommand*>(cmd) != nullptr);
        //bool isTimed = (typeid(*cmd) == typeid(TimeoutCommand));
        if (isTimed) {
            cmd = smash.CreateCommand(static_cast<TimeoutCommand&>(*cmd).getCmdToRun());
        }
        bool isPipe = (dynamic_cast<PipeCommand*>(cmd) != nullptr);
        //bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
        bool isRedir = (dynamic_cast<PipeCommand*>(cmd) != nullptr);
        //bool isRedir = (typeid(*cmd) == typeid(RedirectionCommand));
        // bool isTimed = (typeid(*cmd) == typeid(TimeoutCommand));
        // if (isTimed) {
        //     cmd = smash.CreateCommand(static_cast<TimeoutCommand&>(*cmd).getCmdToRun());
        // }
        // bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
        // bool isRedir = (typeid(*cmd) == typeid(RedirectionCommand));
        delete cmd;
        if (isPipe || isRedir) {
            res = killpg(pid, SIGKILL);
        }
        else {
            res = kill(pid, SIGKILL);
        }
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
    pid_t smashPID = smash.getSmashPid();
    Command* cmd = smash.CreateCommand(entry->getCmd().c_str());
    bool isTimed = (dynamic_cast<TimeoutCommand*>(cmd) != nullptr);
    //bool isTimed = (typeid(*cmd) == typeid(TimeoutCommand));
    if (isTimed) {
        cmd = smash.CreateCommand(static_cast<TimeoutCommand&>(*cmd).getCmdToRun());
    }
    bool isPipe = (dynamic_cast<PipeCommand*>(cmd) != nullptr);
    //bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
    bool isRedir = (dynamic_cast<PipeCommand*>(cmd) != nullptr);
    // bool isTimed = (typeid(*cmd) == typeid(TimeoutCommand));
    // if (isTimed) {
    //     cmd = smash.CreateCommand(static_cast<TimeoutCommand&>(*cmd).getCmdToRun());
    // }
    // bool isPipe = (typeid(*cmd) == typeid(PipeCommand));
    // bool isRedir = (typeid(*cmd) == typeid(RedirectionCommand));
    delete cmd;
    int res = -1;
    smash.removeFinishedJobsFromList();
    if (pid != smashPID) {
        if (isPipe || isRedir) {
            pid_t pgid = getpgid(pid);
            res = killpg(pgid, SIGKILL);
        }
        else {
            res = kill(pid, SIGKILL);
        }
        if (res != 0) {
//            perror("smash error: kill failed");
            return;
        }
    }
    else {
        return;
//        /// if we got here there is a problem
//        res = kill(pid, SIGSTOP);
//        if (res != 0) {
//            perror("smash error: kill failed");
//            return;
//        }
    }
    string cmdStr = entry->getCmd();
    cout << "smash: " << cmdStr << " timed out!" << endl;
    if (!smash.isTimedEmpty()) {
        int alarm_duration = smash.getMinTimeLeft();
        alarm(alarm_duration);
    }
}