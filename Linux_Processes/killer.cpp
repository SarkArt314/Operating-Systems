
#include <bits/stdc++.h>
#include <dirent.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>

using namespace std;

static pid_t selfPid() { return getpid(); }

vector<pid_t> findPidsByName(const string &name) {
    vector<pid_t> result;
    DIR *proc = opendir("/proc");
    if (!proc) return result;
    struct dirent *entry;
    while ((entry = readdir(proc)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;
        string pidStr = entry->d_name;
        pid_t pid = (pid_t)stoi(pidStr);
        if (pid == selfPid()) continue;
        string commPath = "/proc/" + pidStr + "/comm";
        ifstream commFile(commPath);
        if (commFile.is_open()) {
            string comm;
            getline(commFile, comm);
            if (comm == name) {
                result.push_back(pid);
                continue;
            }
        }
        string cmdPath = "/proc/" + pidStr + "/cmdline";
        ifstream cmdFile(cmdPath);
        if (!cmdFile.is_open()) continue;
        string cmdline;
        getline(cmdFile, cmdline, '\0');
        if (!cmdline.empty()) {
            string first = cmdline;
            size_t slash = first.find_last_of('/');
            string base = (slash == string::npos) ? first : first.substr(slash + 1);
            if (base == name) result.push_back(pid);
        }
    }
    closedir(proc);
    sort(result.begin(), result.end());
    result.erase(unique(result.begin(), result.end()), result.end());
    return result;
}

bool killPid(pid_t pid) {
    if (kill(pid, SIGTERM) == 0) {
        for (int i = 0; i < 10; ++i) {
            if (kill(pid, 0) == -1) break;
            usleep(10000);
        }
        if (kill(pid, 0) == 0) {
            if (kill(pid, SIGKILL) != 0) return false;
        }
        return true;
    } else {
        return false;
    }
}

vector<string> splitCsv(const string &s) {
    vector<string> out;
    string cur;
    for (char c : s) {
        if (c == ',') {
            if (!cur.empty()) {
                size_t a = cur.find_first_not_of(" \"'");
                size_t b = cur.find_last_not_of(" \"'");
                if (a != string::npos && b != string::npos && b >= a)
                    out.push_back(cur.substr(a, b - a + 1));
                else if (a != string::npos)
                    out.push_back(cur.substr(a));
                cur.clear();
            }
        } else cur.push_back(c);
    }
    if (!cur.empty()) {
        size_t a = cur.find_first_not_of(" \"'");
        size_t b = cur.find_last_not_of(" \"'");
        if (a != string::npos && b != string::npos && b >= a)
            out.push_back(cur.substr(a, b - a + 1));
        else if (a != string::npos)
            out.push_back(cur.substr(a));
    }
    return out;
}

int main(int argc, char **argv) {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    string optId;
    string optName;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i];
        if (a == "--id" && i + 1 < argc) {
            optId = argv[++i];
        } else if (a == "--name" && i + 1 < argc) {
            optName = argv[++i];
        } else {
        }
    }

    if (!optId.empty()) {
        pid_t pid = (pid_t)stol(optId);
        if (pid == selfPid()) {
            cout << "Refusing to kill self (pid " << pid << ")\n";
            return 1;
        }
        cout << "Attempting to kill pid " << pid << ".\n";
        if (killPid(pid)) {
            cout << "Killed pid " << pid << "\n";
            return 0;
        } else {
            cout << "Failed to kill pid " << pid << " (maybe no such process or permission denied)\n";
            return 2;
        }
    } else if (!optName.empty()) {
        cout << "Searching for processes named '" << optName << "'.\n";
        auto pids = findPidsByName(optName);
        if (pids.empty()) {
            cout << "No processes found with name '" << optName << "'\n";
            return 3;
        }
        int success = 0;
        for (pid_t pid : pids) {
            cout << "Killing pid " << pid << " (name " << optName << ").\n";
            if (killPid(pid)) {
                cout << "Killed " << pid << "\n";
                ++success;
            } else {
                cout << "Failed to kill " << pid << "\n";
            }
        }
        cout << "Summary: attempted " << pids.size() << ", killed " << success << "\n";
        return (success > 0) ? 0 : 4;
    } else {
        const char *env = getenv("PROCTOKILL");
        if (!env) {
            cout << "No arguments and PROCTOKILL not set. Nothing to do.\n";
            return 5;
        }
        string envs(env);
        auto names = splitCsv(envs);
        if (names.empty()) {
            cout << "PROCTOKILL is empty or malformed.\n";
            return 6;
        }
        int totalAttempt = 0, totalKilled = 0;
        for (auto &name : names) {
            if (name.empty()) continue;
            cout << "Looking for processes named '" << name << "' from PROCTOKILL.\n";
            auto pids = findPidsByName(name);
            if (pids.empty()) {
                cout << "  none found for '" << name << "'\n";
                continue;
            }
            for (pid_t pid : pids) {
                ++totalAttempt;
                cout << "  killing pid " << pid << ".\n";
                if (killPid(pid)) {
                    cout << "  killed " << pid << "\n";
                    ++totalKilled;
                } else {
                    cout << "  failed to kill " << pid << "\n";
                }
            }
        }
        cout << "PROCTOKILL summary: attempted " << totalAttempt << ", killed " << totalKilled << "\n";
        return (totalKilled > 0) ? 0 : 7;
    }
}
