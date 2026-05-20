#include <cstring>
#include <ctime>
#include <iostream>

#include "commands.h"
#include "constants.h"
#include "given.h"
#include "pa3_task.h"
#include "structures.h"

using namespace std;

RepositoryManagement repoManagement;
UserManagement userManagement;

time_t make_time_from_string(const char *str) {
  tm buf;
  if (!strptime(str, "%FT%TZ", &buf)) {
    cout << "Invalid timestamp, expected RFC3339 format "
            "(YYYY-MM-DDTHH:MM:SSZ)."
         << endl;
    return -1;
  }
  buf.tm_isdst = 0;

  return timegm(&buf);
}

void find_commit_in_repository_by_hash(Commit *&out, const Repository *repo,
                                       const char *hash) {
  if (strlen(hash) != 40) {
    cout << "Invalid SHA1 hash hex digest (must be 20 bytes in length)" << endl;
    return;
  }

  SHA1 sha;
  sha1_from_hash_str(sha, hash);

  for (Commit *ptr = repo->commits; ptr; ptr = ptr->next) {
    if (hashes_equal(ptr->hash, sha)) {
      out = ptr;
      return;
    }
  }

  for (int i = 0; i < repo->numBranches; i++)
    for (Commit *ptr = repo->branches[i]->head; ptr; ptr = ptr->next)
      if (hashes_equal(ptr->hash, sha)) {
        out = ptr;
        return;
      }

  cout << "Commit " << hash << " not found in any branch of ";
  print(*repo);
  cout << "." << endl;
}

void find_user_by_name(User *&out, const char *name) {
  for (User *ptr = userManagement.head; ptr; ptr = ptr->next)
    if (!strcmp(ptr->name, name)) {
      out = ptr;
      return;
    }

  cout << "User " << name << " does not exist." << endl;
}

void find_repository_by_fqn(Repository *&out, const char *fqn) {
  char *buf = new char[strlen(fqn) + 1];
  strcpy(buf, fqn);

  const char *owner = strtok(buf, "/");
  const char *repo = strtok(nullptr, "/");

  for (int i = 0; i < repoManagement.numRepos; i++)
    if (!strcmp(repoManagement.repos[i]->name, repo) &&
        !strcmp(repoManagement.repos[i]->owner->name, owner)) {
      out = repoManagement.repos[i];

      delete[] buf;
      return;
    }

  // FIX: use-after-free — owner/repo pointers dangle after delete[] buf
  //      moved delete[] buf after the error message
  if (!out) {
    cout << "Specified repository " << owner << "/" << repo
         << " does not exist." << endl;
  }

  delete[] buf;
}

multimap<string, command_t> commands = {
    {"register",
     {"[user]", "Registers a new user.", 1,
      [](const vector<string> &args) -> void {
        cout << "Attempting to register new user: " << args[0] << endl;
        const User *out;
        if (!((out = register_new_user(userManagement, args[0].c_str())))) {
          cout << "Registration failed." << endl;
          return;
        }

        cout << "Registration successful." << endl;
        cout << "User: ";
        print(*out);
        cout << endl;
      }}},

    {"repocreate",
     {"[owner] [repository] [timestamp]",
      "Creates a new repository with the specified name owned by the specified "
      "user and timestamp.",
      3,
      [](const vector<string> &args) -> void {
        User *owner = nullptr;
        find_user_by_name(owner, args[0].c_str());

        if (!owner) {
          return;
        }

        cout << "Attempting to create repository: " << args[0] << "/" << args[1]
             << endl;
        const time_t timestamp = make_time_from_string(args[2].c_str());
        if (timestamp == -1)
          return;
        const int i = create_repository(repoManagement, owner, args[1].c_str(),
                                        timestamp);
        if (i == -1) {
          cout << "Repository creation failed." << endl;
          return;
        }

        cout << "Repository creation successful." << endl;
        cout << "Repository is at index " << i
             << " in the array of repositories." << endl;
      }}},

    {"branchcreate",
     {"[repository (FQN)] [branch] [creator]",
      "Creates a new branch with the specified name in the specified "
      "repository owned by the specified user.",
      3,
      [](const vector<string> &args) -> void {
        User *creator = nullptr;
        find_user_by_name(creator, args[2].c_str());

        if (!creator)
          return;

        cout << "Attempting to create branch " << args[1] << " in repository "
             << args[0] << endl;
        if (!create_branch(repoManagement, const_cast<char *>(args[0].c_str()),
                           args[1].c_str(), creator, nullptr)) {
          cout << "Branch creation failed." << endl;
          return;
        }

        cout << "Branch creation successful" << endl;
      }}},

    {"branchcreate",
     {"[repository FQN] [branch] [creator] [commit]",
      "Creates a branch with the specified name in the specified repository "
      "owned by the specified user, originating from the specified commit.",
      4,
      [](const vector<string> &args) -> void {
        User *creator = nullptr;
        find_user_by_name(creator, args[2].c_str());

        if (!creator)
          return;

        cout << "Attempting to create branch " << args[1] << " in repository "
             << args[0] << " from commit " << args[3] << endl;

        Repository *repo = nullptr;
        find_repository_by_fqn(repo, args[0].c_str());

        if (!repo)
          return;

        Commit *commit = nullptr;
        find_commit_in_repository_by_hash(commit, repo, args[3].c_str());

        if (!commit)
          return;
        if (!create_branch(repoManagement, const_cast<char *>(args[0].c_str()),
                           args[1].c_str(), creator, commit)) {
          cout << "Branch creation failed." << endl;
          return;
        }

        cout << "Branch creation successful" << endl;
      }}},

    {"userinfo",
     {"[user]", "Displays detailed information of a user.", 1,
      [](const vector<string> &args) -> void {
        User *user = nullptr;
        find_user_by_name(user, args[0].c_str());

        if (!user)
          return;

        print(*user, true);
        cout << endl;
      }}},

    {"repoinfo",
     {"[repository (FQN)]", "Displays detailed information of a repository.", 1,
      [](const vector<string> &args) -> void {
        Repository *repo = nullptr;
        find_repository_by_fqn(repo, args[0].c_str());

        if (!repo)
          return;

        print(*repo, true);
        cout << endl;
      }}},

    {"repotransfer",
     {"[repository (FQN)] [newowner]",
      "Transfers ownership of a repository to someone else.", 2,
      [](const vector<string> &args) -> void {
        Repository *repo = nullptr;
        find_repository_by_fqn(repo, args[0].c_str());

        if (!repo)
          return;

        if (!transfer_ownership(userManagement, repoManagement,
                                repo->owner->name, args[1].c_str(),
                                repo->name)) {
          cout << "Ownership transfer was unsuccessful." << endl;
          return;
        }

        cout << "Repository " << args[0] << " has been transferred to "
             << args[1] << "." << endl;
      }}},

    {"users",
     {"", "Lists registered users.", 0,
      [](const vector<string> &_) -> void {
        for (User *ptr = userManagement.head; ptr; ptr = ptr->next) {
          print(*ptr);
          cout << endl;
        }
      }}},

    {"repos",
     {"", "Lists repositories.", 0,
      [](const vector<string> &_) -> void {
        for (int i = 0; i < repoManagement.numRepos; i++) {
          print(*repoManagement.repos[i]);
          cout << endl;
        }
      }}},

    {"push",
     {"[branch (fully-qualified name)] [author] [message] [timestamp]",
      "Pushes a commit to the specified branch.", 4,
      [](const vector<string> &args) -> void {
        time_t timestamp = make_time_from_string(args[3].c_str());

        User *author = nullptr;
        find_user_by_name(author, args[1].c_str());

        if (!author)
          return;

        const size_t branchIndex = args[0].find(":");
        const char *branchName = nullptr;

        // FIX: branch string was a temporary whose c_str() dangled after scope;
        //      hoist it so it lives until add_commit returns
        string branch;
        if (branchIndex != string::npos) {
          // FIX: substr(branchIndex) included the ":" character; skip it
          branch = args[0].substr(branchIndex + 1);
          branchName = branch.c_str();
        }

        const string repoName = branchIndex == string::npos
                                    ? args[0]
                                    : args[0].substr(0, branchIndex);

        add_commit(repoManagement, author, const_cast<char *>(repoName.c_str()),
                   branchName, args[2].c_str(), timestamp);
      }}},

    {"prcreate",
     {"[author] [source (FQN)] [target (FQN)] [title]",
      "Creates a pull request to the specified branch of a repository.", 4,
      [](const vector<string> &args) -> void {
        User *author = nullptr;
        find_user_by_name(author, args[0].c_str());

        if (!author)
          return;

        if (!create_pull_request(repoManagement, args[3].c_str(), author,
                                 const_cast<char *>(args[1].c_str()),
                                 const_cast<char *>(args[2].c_str())))
          cout << "Pull request creation was unsuccessful." << endl;
      }}},
    {"forkrepo",
     {"[owner] [source repository (FQN)] [repository]", "Forks a repository.",
      3,
      [](const vector<string> &args) -> void {
        // FIX: source FQN is args[1], not args[2]; extract repo name via second strtok
        char *repoFQN = const_cast<char *>(args[1].c_str());
        const char *original = strtok(repoFQN, "/");
        const char *repoName = strtok(nullptr, "/");

        User *originalOwner = nullptr;
        find_user_by_name(originalOwner, original);

        if (!originalOwner)
          return;

        if (!fork_repository(userManagement, repoManagement, originalOwner,
                             args[0].c_str(), repoName))
          cout << "Repository forking was unsuccessful." << endl;
      }}},
    {"prmerge",
     {"[repository (FQN)] [pr number] [merge mode] [timestamp]",
      "Merges a pull request in the specified repository with a merge "
      "strategy at the specified timestamp.",
      4,
      [](const vector<string> &args) -> void {
        PullRequestMergeStrategy strategy =
            static_cast<PullRequestMergeStrategy>(stoi(args[2]));
        // FIX: PR number is args[1], not args[2] (was passing merge mode as PR number)
        merge_pull_request(repoManagement, const_cast<char *>(args[0].c_str()),
                           stoi(args[1]),
                           make_time_from_string(args[3].c_str()), strategy);
      }}},
    {"deregister",
     {"[username]", "De-registers a user.", 1,
      [](const vector<string> &args) -> void {
        deregister_user(userManagement, repoManagement, args[0].c_str());
      }}},

    {"prinfo",
     {"[repository (FQN)] [pr number]",
      "Shows information about a pull request in a repository.", 2,
      [](const vector<string> &args) -> void {
        Repository *repo = nullptr;
        find_repository_by_fqn(repo, args[0].c_str());

        if (!repo)
          return;

        int prNumber = stoi(args[1]);
        // FIX: missing braces and return — fell through to null dereference
        if (prNumber > repo->numPrs || prNumber <= 0) {
          cout << "Pull request number out of range for reposiotry ";
          print(*repo);
          cout << endl;
          return;
        }

        print(*repo->prs[prNumber - 1], true);
        cout << endl;
      }}},

    {"branchinfo",
     {"[branch (FQN)]", "Displays detailed information about a branch.", 1,
      [](const vector<string> &args) -> void {
        char *branchFQN = const_cast<char *>(args[0].c_str());
        char *repoFQN = strtok(branchFQN, ":");
        char *branch = strtok(nullptr, ":");

        Repository *ptr = nullptr;
        find_repository_by_fqn(ptr, repoFQN);

        if (!ptr)
          return;

        if (!strcmp(branch, "main")) {
          Branch *main = new Branch;
          main->creator = ptr->owner;
          main->head = ptr->commits;
          main->repo = ptr;

          cout << "Branch ";
          print(*main, true);

          delete main;

          return;
        }

        Branch *pBr = nullptr;
        for (int i = 0; i < ptr->numBranches; i++)
          if (!strcmp(ptr->branches[i]->name, branch)) {
            pBr = ptr->branches[i];
            break;
          }

        if (!pBr) {
          cout << "Branch ";
          print(*ptr);
          cout << ":" << branch << " does not exist.";

          return;
        }

        cout << "Branch ";
        print(*pBr, true);
      }}},
};
