#include <ctime>
#include <iostream>

#include "given.h"
#include "pa3_task.h"
#include "sha1.h"
#include "structures.h"

using namespace std;

void merge_pull_request(RepositoryManagement &repoManagement, char *repoName,
                        int prNumber, time_t timestamp,
                        PullRequestMergeStrategy strategy) {
  switch (strategy) {
  case MERGE_COMMIT:
    merge_pull_request_mergecommit(repoManagement, repoName, prNumber,
                                   timestamp);
    break;
  case REBASE_MERGE:
    // FIX: was calling mergecommit instead of rebasemerge
    merge_pull_request_rebasemerge(repoManagement, repoName, prNumber,
                                   timestamp);
    break;
  case SQUASH_AND_MERGE:
    merge_pull_request_squashmerge(repoManagement, repoName, prNumber,
                                   timestamp);
  }
}

void print(const Branch &branch, bool details) {
  digest(branch.head->hash);

  print(*branch.repo);
  // FIX: separator was ";" instead of "/" per spec output format
  cout << "/" << branch.name << ": ";
  print_sha(branch.head->hash);

  cout << " (created by " << branch.creator->name << ")";

  if (details) {
    cout << "Log: " << endl;

    for (Commit *ptr = branch.head; ptr; ptr = ptr->next) {
      print(*ptr);
      cout << endl;
    }
  }
}

void print(Commit &commit) {
  char buf[26];
  strftime(buf, sizeof(buf), "%d %b %Y %H:%M:%S GMT",
           gmtime(&commit.timestamp));

  cout << "commit ";
  print_sha(commit.hash);
  cout << endl;

  cout << "Author: ";
  print(*commit.author);
  cout << endl;

  cout << "Date: " << buf << endl << endl;
  cout << commit.message << endl << endl;
}

void print(const PullRequest &pr, bool details) {
  cout << "#" << pr.id << ": " << pr.title;

  if (details) {
    cout << " in ";
    print(*pr.repo);
    cout << endl;

    cout << "Author: ";
    print(*pr.author);
    cout << endl;

    cout << "From Branch: ";
    print(*pr.fromBranch);
    cout << endl;

    cout << "To Branch: ";
    if (pr.toBranch)
      print(*pr.toBranch);
    else {
      print(*pr.repo);
      cout << ":main";
    }
    cout << endl;

    cout << "Status: ";
    switch (pr.status) {
    case OPEN:
      cout << "Open";
      break;
    case CLOSED:
      cout << "Closed";
      break;
    case MERGED:
      cout << "Merged";
      break;
    default:
      break;
    }
  }
}

void print(const Repository &repo, bool details) {
  cout << repo.owner->name << "/" << repo.name;

  if (details) {
    cout << endl;

    cout << "Commits: " << endl;
    for (Commit *ptr = repo.commits; ptr; ptr = ptr->next) {
      print(*ptr);
      cout << endl;
    }
    cout << endl;

    cout << "Branches (" << repo.numBranches << "):" << endl;
    for (int i = 0; i < repo.numBranches; i++) {
      print(*repo.branches[i]);
      cout << endl;
    }
    cout << endl;

    cout << "Forks (" << repo.numForks << "):" << endl;
    for (int i = 0; i < repo.numForks; i++) {
      print(*repo.forks[i]);
      cout << endl;
    }
    cout << endl;

    cout << "Pull Requests (" << repo.numPrs << "):" << endl;
    for (int i = 0; i < repo.numPrs; i++) {
      print(*repo.prs[i]);
      cout << endl;
    }
    cout << endl;
  }
}

void print(const User &user, bool details) {
  cout << user.name;

  if (details) {
    cout << endl << endl;

    cout << "Repositories (" << user.numRepos << "):" << endl;
    for (int i = 0; i < user.numRepos; i++) {
      print(*user.repos[i]);
      cout << endl;
    }
  }
}

bool commits_equal(const Commit &l, const Commit &r) {
  return hashes_equal(l.hash, r.hash);
}
