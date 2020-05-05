#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>


using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define BUFFER_SIZE (4096)

class Command {
// TODO: Add your data members
private:
    string cmd;
    bool takes_cpu;
public:
    Command(const char* cmd_line, bool takesCPU);
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed

    const char* getCommand() const;
    bool takesCPU() const;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(const char* cmd_line, bool takesCPU);
    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
public:
    ExternalCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
    // TODO: Add your data members
    string firstCmd;
    string secondCmd;
    bool redirectStdOut;
public:
    PipeCommand(const char* cmd_line, bool takes_cpu);
    virtual ~PipeCommand() {}
    void execute() override;
};

class RedirectionCommand : public Command {
    // TODO: Add your data members
private:
    string innerCmd;
    string file;
    bool isOverWrite;
    int stdoutFD;
public:
    RedirectionCommand(const char* cmd_line, char** args, int numOfArgs, bool takes_cpu, bool isOverWrite);
    virtual ~RedirectionCommand() {}
    void execute() override;
    bool isForkable();
    //void prepare() override;
    //void cleanup() override;
};

class ChpromptCommand : public BuiltInCommand {
private:
    string new_prompt;

public:
    ChpromptCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu=false);
    ChpromptCommand(const ChpromptCommand&) = default;
    ChpromptCommand&operator=(const ChpromptCommand&) = default;
    ~ChpromptCommand() = default;

    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
private:
    string newWD;
    string currentWD;
public:
    ChangeDirCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu=false);
    ChangeDirCommand(const ChangeDirCommand&) = default;
    ChangeDirCommand&operator=(const ChangeDirCommand&) = default;
    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
private:
    /// No private fields as of yet
public:
    GetCurrDirCommand(const string cmd, bool takes_cpu=false);
    virtual ~GetCurrDirCommand() {}
    GetCurrDirCommand(const GetCurrDirCommand&) = default;
    GetCurrDirCommand&operator=(const GetCurrDirCommand&) = default;

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const string cmd, bool takes_cpu=false);
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
private:
    JobsList* jobs;
    bool isKilling;
public:
    QuitCommand(const string cmd, char** args, int numOfStrings, bool takes_cpu, JobsList* jobs);
    virtual ~QuitCommand() {}
    void execute() override;
};

//class CommandsHistory {
// protected:
//  class CommandHistoryEntry {
//	  // TODO: Add your data members
//  };
// // TODO: Add your data members
// public:
//  CommandsHistory();
//  ~CommandsHistory() {}
//  void addRecord(const char* cmd_line);
//  void printHistory();
//};
//
//class HistoryCommand : public BuiltInCommand {
// // TODO: Add your data members
// public:
//  HistoryCommand(const char* cmd_line, CommandsHistory* history);
//  virtual ~HistoryCommand() {}
//  void execute() override;
//};

class JobsList {
public:
    class JobEntry {
    private:
        // TODO: Add your data members
        int jobID;
        const pid_t jobPID;
        const string cmd;
        const time_t start;
        bool isStopped;
        // TODO: Add your data members
    public:
        JobEntry(int jobID, pid_t pid, const string& cmd, bool isStopped);
//        JobEntry(const JobEntry&) = default;
//        JobEntry&operator=(const JobEntry&)= default;
        ~JobEntry() = default;

        int getJobID() const;
        pid_t getJobPID() const;
        string getJobCmd() const;
        time_t getJobStartTime() const;
        bool isJobStopped() const;

        void setJobID(int id);
        void stopJob();
        void resumeJob();
        /// need to add a printing function/operator
    };

private:
    vector<JobEntry*> jobs;
    int jobsCount;

public:
    JobsList();
    ~JobsList();
    void addJob(Command* cmd, pid_t pid, bool isStopped = false);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    JobEntry * getJobById(int jobId);
    void removeJobById(int jobId);
    JobEntry * getLastJob(int* lastJobId);
    JobEntry *getLastStoppedJob(int *jobId);
// TODO: Add extra methods or modify exisitng ones as needed

    const int getJobsCount() const;
    void updateJobsCount();
    int getTotalJobs() const;
    void killThisFucker(int jobId);
};
ostream&operator<<(ostream& os, const JobsList::JobEntry& jobEntry);

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
private:
    JobsList* jobs;
public:
    JobsCommand(const char* cmd_line, JobsList* jobs, bool takes_cpu);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
private:
    int signal;
    int jobID;
    pid_t jobPID;
    JobsList* jobs;

public:
    KillCommand(const char* cmd_line, char** args, int numOfArgs, JobsList* jobs, bool takes_cpu);
    virtual ~KillCommand() = default;
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
private:
    int jobID;
    JobsList* jobs;
    bool jobIDGiven;
public:
    ForegroundCommand(const char *cmd_line, char** args, int numOfArgs, JobsList *jobs, bool takes_cpu);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    // TODO: Add your data members
private:
    int jobID;
    int jobToStopID;
    JobsList* jobs;
    JobsList::JobEntry* jobToStop;
public:
    BackgroundCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu, JobsList* jobs);
    virtual ~BackgroundCommand() {}
    void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
private:
    string src;
    string dest;
public:
    CopyCommand(const char* cmd_line, char** args, int numOfArgs, bool takes_cpu);
    virtual ~CopyCommand() {}
    void execute() override;
};

// TODO: add more classes if needed
// maybe chprompt , timeout ?


class TimeoutCommand;
class TimeoutEntry {
private:
    string cmd;
    pid_t pid;
    time_t start;
    int duration;

public:
    TimeoutEntry(const string cmd_line, pid_t pid, int duration);
    ~TimeoutEntry() = default;
    string getCmd() const;
    pid_t getPID() const;
    time_t getTimeLeft() const;

    bool operator<(const TimeoutEntry& e) const {
        int timeLeft1 = this->getTimeLeft();
        int timeLeft2 = e.getTimeLeft();
        return timeLeft1 < timeLeft2;
    }
};


class TimeoutCommand : public BuiltInCommand {
private:
    string cmdToRun;
    time_t start;
    int duration;

public:
    TimeoutCommand(const char* cmd_line, char** args, int numOfArgs, bool takes_cpu);
    ~TimeoutCommand() = default;

    int getDuration() const;
    const char* getCmdToRun() const;
    void execute();
};

class comparePtrs {
public:
    bool operator()(TimeoutEntry* e1, TimeoutEntry* e2) {return *e1 < *e2;}
};

class SmallShell {
private:
    // TODO: Add your data members
    string prompt;
    string lastWD;
    JobsList* jobList;
    vector<TimeoutEntry*> timedList;
    JobsList::JobEntry* currentJob;
    pid_t smashPID;
    bool fromPipe;
    bool fromRedir;
    SmallShell();
public:
    Command *CreateCommand(const char* cmd_line);
    SmallShell(SmallShell const&)      = delete; // disable copy ctor
    void operator=(SmallShell const&)  = delete; // disable = operator
    static SmallShell& getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }
    ~SmallShell();
    pid_t getSmashPid() const;
    void executeCommand(const char* cmd_line);
    void setPrompt(string newPrompt);
    string getPrompt() const;
    void setLastWD(string path);
    string getLastWD() const;
    // TODO: add extra methods as needed
    void addTimedCommand(const string cmd, pid_t pid, int duration);
    int getMinTimeLeft();
    TimeoutEntry* getEntryToKill();
    JobsList::JobEntry* getCurrentJob();
    bool isTimedEmpty() const;
    void updateCurrentJob(JobsList::JobEntry* job);
    void clearCurrentJob(bool deleteEntry);
    bool jobInList(int jobId) const;
    void addSleepingJob(JobsList::JobEntry* job);
    void markFromPipe() {fromPipe = true;}
    void markFromRedir() {fromRedir = true;}
};

#endif //SMASH_COMMAND_H_