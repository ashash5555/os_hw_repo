#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <time.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "

#define EXEC(path, arg) \
  execvp((path), (arg));

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

// TODO: Add your implementation for classes in Commands.h



                                                 ///Command///
///==================================================================================================================///
Command::Command(const char* cmd_line, bool takes_cpu) : cmd(cmd_line), takes_cpu(takes_cpu) {}
const char* Command::getCommand() const { return cmd.c_str();}
bool Command::takesCPU() const { return takes_cpu;}
///==================================================================================================================///


                                                ///JobsList///
///==================================================================================================================///
JobsList::JobEntry::JobEntry(int jobID, pid_t pid, const string& cmd, bool isStopped) : jobID(jobID), jobPID(pid),
                                                          cmd(cmd), start(time(nullptr)), isStopped(isStopped) {}
int JobsList::JobEntry::getJobID() const { return jobID;}
pid_t JobsList::JobEntry::getJobPID() const { return jobPID;}
string JobsList::JobEntry::getJobCmd() const { return cmd;}
time_t JobsList::JobEntry::getJobStartTime() const { return start;}
bool JobsList::JobEntry::isJobStopped() const { return isStopped;}
void JobsList::JobEntry::setJobID(int id) {jobID = id;}
void JobsList::JobEntry::stopJob() {isStopped = true;}
void JobsList::JobEntry::resumeJob() {isStopped = false;}

/// Prints the job entry as one line: '[<job-id>] <command> : <process-id> <seconds-elapsed> secs (stopped)'
/// \param os - ostream object (file descriptor)
/// \param jobEntry - jobEntry to print
/// \return os
ostream&operator<<(ostream& os, const JobsList::JobEntry& jobEntry) {
    pid_t pid = jobEntry.getJobPID();
    string cmd = jobEntry.getJobCmd();
    int jobID = jobEntry.getJobID();
    bool isStopped = jobEntry.isJobStopped();
    time_t start = jobEntry.getJobStartTime();
    time_t current = time(nullptr);
    int timeElapsed = difftime(current, start) + 0.5;

    os << "[" << jobID << "] " << cmd << " : " << pid << " " << timeElapsed << " secs";

    if (isStopped) {
        os << " (" << "stopped" << ")";
    }

    os << endl;
    return os;
}



JobsList::JobsList() : jobs(vector<JobsList::JobEntry*>()), numOfJobs(0), lastJob(nullptr), lastJobStopped(nullptr) {}
JobsList::~JobsList() {
    if (lastJob) {
        delete lastJob;
        lastJob = nullptr;
    }

    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    vector<JobsList::JobEntry*>::iterator tempIt = it;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            delete *it;
            *it = nullptr;
            /// because iterator is undefined after erase
            tempIt = it;
            jobs.erase(it);
            it = tempIt;
        }
    }
}

void JobsList::addJob(Command *cmd, pid_t pid, bool isStopped) {
    this->numOfJobs++;
    int jobID = numOfJobs;
    const char* cmdStr = cmd->getCommand();

    JobEntry* jobToAdd = new JobEntry(jobID, pid, cmdStr, isStopped);
    jobs.push_back(jobToAdd);
}

void JobsList::printJobsList() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    for (; it != jobs.end(); ++it) {
        if (*it) {
            cout << *it;
        }
    }
}

void JobsList::killAllJobs() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    vector<JobsList::JobEntry*>::iterator tempIt = it;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            kill(pid, SIGKILL);    /// check return status?

            delete *it;
            *it = nullptr;
            /// because iterator is undefined after erase
            tempIt = it;
            jobs.erase(it);
            it = tempIt;

            this->numOfJobs--;
        }
    }
}

void JobsList::removeFinishedJobs() {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    vector<JobsList::JobEntry*>::iterator tempIt = it;
    int res;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            pid_t pid = (*it)->getJobPID();
            /// waitpid with WNOHANG returns the pid if the process finished and 0 if it is still running
            res = waitpid(pid, nullptr, WNOHANG);
            if (res > 0) {
                /// gets here if this process finished
                delete *it;
                *it = nullptr;
                /// because iterator is undefined after erase
                tempIt = it;
                jobs.erase(it);
                it = tempIt;

                this->numOfJobs--;
            }
        }
    }
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    vector<JobsList::JobEntry*>::iterator it = jobs.begin();
    int itJobID;
    for (; it != jobs.end(); ++it) {
        if (*it) {
            itJobID = (*it)->getJobID();
            if (itJobID == jobId) {
                return *it;
            }
        }
    }
    /// gets here if there is no job with the given id
    return nullptr;
}
///==================================================================================================================///
















SmallShell::SmallShell() {
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {

  string cmd_s = string(cmd_line);
  if (cmd_s.find("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (cmd_s.)
  .....
  else {
    return new ExternalCommand(cmd_line);
  }
  
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  // Command* cmd = CreateCommand(cmd_line);
  // cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}