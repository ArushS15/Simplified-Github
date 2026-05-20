#include <algorithm>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <utility>

#include "commands.h"

#if defined(__ZINC__) || defined(__linux__) || defined(__APPLE__)
#include <unistd.h>
#elif defined(_WIN32) || defined(WIN32)
#include <io.h>
#endif

using namespace std;

User *ghost = nullptr;

void cleanup(UserManagement &users, RepositoryManagement &repos) {
  for (int i = 0; i < repos.numRepos; i++) {
    Repository *ptr = repos.repos[i];

    delete[] ptr->forks;

    for (int j = 0; j < ptr->numBranches; j++) {
      Branch *branch = ptr->branches[j];

      Commit *curr = branch->head;
      while (curr) {
        Commit *next = curr->next;
        delete curr;
        curr = next;
      }

      delete branch;
    }
    delete[] ptr->branches;

    for (int j = 0; j < ptr->numPrs; j++)
      delete ptr->prs[j];
    delete[] ptr->prs;

    for (Commit *commit = ptr->commits; commit;) {
      Commit *next = commit->next;
      delete commit;
      commit = next;
    }

    delete ptr;
  }

  delete[] repos.repos;

  for (User *ptr = users.head; ptr;) {
    User *next = ptr->next;

    delete[] ptr->repos;
    delete ptr;

    ptr = next;
  }
}

int main() {
#ifdef __ZINC__
  // redirect STDERR to STDOUT for memory-related test cases
  dup2(STDOUT_FILENO, STDERR_FILENO);
#endif

#if defined(__ZINC__) || defined(__linux__) || defined(__APPLE__)
  const bool interactive = isatty(STDIN_FILENO);
#elif defined(_WIN32) || defined(WIN32)
  const bool interactive = _isatty(_fileno(stdin))
#endif

  ghost = new User;
  strcpy(ghost->name, "ghost");
  ghost->next = nullptr;

  cout << "-----------------------------------------------------" << endl;
  cout << "| Welcome to Version Control System 2011 (VCS2011)! |" << endl;
  cout << "-----------------------------------------------------" << endl;

  cout << endl;

  string command;
  while (true) {
    if (interactive) {
      cout << "vcs2011> " << flush;
    } else {
      cout << "vcs2011>" << endl;
    }
    getline(cin, command);

    if (command.empty())
      continue;

    if (command == "quit") {
      cout << "Quitting..." << endl;
      goto exit;
    }

    if (command == "help") {
      const auto maxname =
          max_element(commands.begin(), commands.end(),
                      [](const pair<string, command_t> &left,
                         const pair<string, command_t> &right) -> bool {
                        return left.first.length() < right.first.length();
                      });
      int namelen = maxname->first.length();

      const auto maxparams = max_element(
          commands.begin(), commands.end(),
          [](const pair<string, command_t> &left,
             const pair<string, command_t> &right) {
            return strlen(left.second.params) < strlen(right.second.params);
          });
      int paramslen = strlen(maxparams->second.params);

      cout << "Version Control System 2011 (VCS2011) Help" << endl;
      cout << endl;

      for (const pair<string, command_t> item : commands) {
        cout << right << setw(namelen) << item.first << "  " << left
             << setw(paramslen) << item.second.params << "  "
             << item.second.desc << endl;
      }

      cout << endl;
      continue;
    }

    vector<string> tokens{};

    size_t pos = 0;
    string tok;
    while ((pos = command.find(" ")) != string::npos) {
      tok = command.substr(0, pos);
      tokens.push_back(tok);
      command.erase(0, pos + 1);
    }
    tokens.push_back(command);

    vector<string> arguments{};
    bool inString = false;

    string strArg;
    for (size_t i = 1; i < tokens.size(); i++) {
      if (tokens[i][0] == '"') {
        strArg += tokens[i].substr(1);
        inString = true;
        continue;
      }

      if (inString && tokens[i][tokens[i].length() - 1] == '"') {
        strArg += " ";
        strArg += tokens[i].substr(0, tokens[i].length() - 1);

        inString = false;
        arguments.push_back(strArg);
        continue;
      }

      if (inString) {
        strArg += " ";
        strArg += tokens[i];
        continue;
      }

      arguments.push_back(tokens[i]);
    }

    const auto range = commands.equal_range(tokens[0]);
    if (range.first == commands.end()) {
      cout << "Invalid command: " << tokens[0] << endl;
      break;
    }

    auto it = range.first;
    for (; it != range.second; ++it) {
      if (arguments.size() == it->second.nParams)
        break;
    }

    if (it == range.second) {
      cout << "No overload of command " << tokens[0]
           << " takes the specified parameters." << endl;
      continue;
    }

    it->second.f(arguments);
  }

exit:
  delete ghost;

  cleanup(userManagement, repoManagement);

  return EXIT_SUCCESS;
}
