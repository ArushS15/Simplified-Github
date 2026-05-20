#ifndef __STRUCTURES_H__
#define __STRUCTURES_H__

#include "constants.h"
#include "sha1.h"

using namespace std;

// ====    Region: Forward Declarations ====

struct Commit;
struct Repository;
struct User;

// ==== Endregion: Forward Declarations ====

// ====    Region: Management Structures ====

struct RepositoryManagement {
  int numRepos;
  Repository **repos;
};

struct UserManagement {
  User *head;
};

// ==== Endregion: Management Structures ====

// ====    Region: Structures ====

struct Branch {
  char name[MAX_BRANCH_NAME_LEN];
  const User *creator;
  Commit *head;
  Repository *repo;
};

struct Commit {
  const User *author;
  char message[MAX_COMMIT_MSG_LEN];
  time_t timestamp;
  SHA1 hash;
  Commit *next;
  Commit *prev;
};

struct PullRequest {
  const User *author;
  int id;
  char title[MAX_PR_TITLE_LEN];
  Repository *repo;
  Branch *fromBranch;
  Branch *toBranch;
  PullRequestStatus status;
};

struct Repository {
  const User *owner;
  char name[MAX_REPO_NAME_LEN];
  int numPrs;
  PullRequest **prs;
  int numForks;
  Repository **forks;
  Commit *commits;
  int numBranches;
  Branch **branches;
};

struct User {
  char name[MAX_USER_NAME_LEN];
  User *next;
  int numRepos;
  Repository **repos;
};

// ==== Endregion: Structures ====

#endif /* __STRUCTURES_H__ */