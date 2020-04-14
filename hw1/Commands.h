#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>


using namespace std;

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

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
 private:
  vector<char*> bashArgs;
 public:
  ExternalCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {}
  void execute() override;
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
  GetCurrDirCommand(const string cmd, char** args, int numOfArgs, bool takes_cpu=false);
  virtual ~GetCurrDirCommand() {}
  GetCurrDirCommand(const GetCurrDirCommand&) = default;
  GetCurrDirCommand&operator=(const GetCurrDirCommand&) = default;

  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const string cmd, char** argg, int numOfArgs, bool takes_cpu=false);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
// TODO: Add your data members public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};

class CommandsHistory {
 protected:
  class CommandHistoryEntry {
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
  void execute() override;
};

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
};
ostream&operator<<(ostream& os, const JobsList::JobEntry& jobEntry);

class JobsCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  JobsCommand(const char* cmd_line, JobsList* jobs);
  virtual ~JobsCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs);
  virtual ~BackgroundCommand() {}
  void execute() override;
};


// TODO: should it really inhirit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
 public:
  CopyCommand(const char* cmd_line);
  virtual ~CopyCommand() {}
  void execute() override;
};

// TODO: add more classes if needed 
// maybe chprompt , timeout ?



class SmallShell {
 private:
  // TODO: Add your data members
  string prompt;
  string lastWD;
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
  void executeCommand(const char* cmd_line);
  void setPrompt(string newPrompt);
  string getPrompt() const;
  void setLastWD(string path);
  string getLastWD() const;
  // TODO: add extra methods as needed
};

#endif //SMASH_COMMAND_H_
