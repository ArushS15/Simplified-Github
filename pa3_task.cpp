#include <cstring>

#include "constants.h"
#include "given.h"
#include "pa3_task.h"
#include "structures.h"

using namespace std;

extern User *ghost;

static int compare_repo_key(const char *own1, const char *repo1,
                            const char *own2, const char *repo2) {
  const int cmp = strcmp(own1, own2);
  if (cmp != 0)
    return cmp;
  return strcmp(repo1, repo2);
}

static User *find_user(UserManagement &userManagement, const char *username) {
  for (User *curr = userManagement.head; curr; curr = curr->next)
    if (strcmp(curr->name, username) == 0)
      return curr; 
  return nullptr;
}

static Repository *find_repository(RepositoryManagement &repoManagement,
                                   const char *ownerName,
                                   const char *repoName) {
  for (int i = 0; i < repoManagement.numRepos; i++)
    if (strcmp(repoManagement.repos[i]->owner->name, ownerName) == 0 &&
        strcmp(repoManagement.repos[i]->name, repoName) == 0)
      return repoManagement.repos[i];
  return nullptr;
}

static Repository *find_repository_by_fqn(RepositoryManagement &repoManagement,
                                          const char *repoFQN) {
  char *buf = new char[strlen(repoFQN) + 1];
  strcpy(buf, repoFQN);

  char *ownerName = strtok(buf, "/");
  char *repoName = strtok(nullptr, "/");
  Repository *repo = nullptr;

  if (ownerName && repoName)
    repo = find_repository(repoManagement, ownerName, repoName);

  delete[] buf;
  return repo;
}

static Branch *find_branch(Repository *repo, const char *branchName) {
  if (branchName == nullptr)
    return nullptr;
  for (int i = 0; i < repo->numBranches; i++)
    if (strcmp(repo->branches[i]->name, branchName) == 0)
      return repo->branches[i];
  return nullptr;
}

static bool is_branch_in_repo(Repository *repo, Branch *branch) {
  if (repo == nullptr || branch == nullptr)
    return false;
  for (int i = 0; i < repo->numBranches; i++)
    if (repo->branches[i] == branch)
      return true;
  return false;
}

static void compute_commit_hash(Commit *commit) {
  SHA1 hasher;
  initialize(hasher);
  input(hasher, commit->author->name, strlen(commit->author->name));
  input(hasher, commit->message, strlen(commit->message));
  input(hasher, commit->timestamp);
  if (commit->next) {
    input(hasher, commit->next->author->name, strlen(commit->next->author->name));
    input(hasher, commit->next->message, strlen(commit->next->message));
    input(hasher, commit->next->timestamp);
  }
  digest(hasher);
  commit->hash = hasher;
}

static void recompute_hashes(Commit *head) {
  if (head == nullptr)
    return;

  Commit *curr = head;
  while (curr->next)
    curr = curr->next;

  while (curr) {
    compute_commit_hash(curr);
    curr = curr->prev;
  }
}

static Commit *clone_commit_chain(const Commit *head) {
  Commit *newHead = nullptr;
  Commit *prev = nullptr;

  for (const Commit *curr = head; curr; curr = curr->next) {
    Commit *copy = new Commit;
    copy->author = curr->author;
    strcpy(copy->message, curr->message);
    copy->timestamp = curr->timestamp;
    copy->hash = curr->hash;
    copy->prev = prev;
    copy->next = nullptr;

    if (prev)
      prev->next = copy;
    else
      newHead = copy;

    prev = copy;
  }

  return newHead;
}

static void delete_commit_chain(Commit *head) {
  while (head) {
    Commit *next = head->next;
    delete head;
    head = next;
  }
}

static void insert_repo_into_management(RepositoryManagement &repoManagement,
                                        Repository *repo) {
  Repository **newRepos = new Repository *[repoManagement.numRepos + 1];
  int insertIndex = 0;

  while (insertIndex < repoManagement.numRepos &&
         compare_repo_key(repoManagement.repos[insertIndex]->owner->name,
                          repoManagement.repos[insertIndex]->name,
                          repo->owner->name, repo->name) < 0) {
    newRepos[insertIndex] = repoManagement.repos[insertIndex];
    insertIndex++;
  }

  newRepos[insertIndex] = repo;

  for (int i = insertIndex; i < repoManagement.numRepos; i++)
    newRepos[i + 1] = repoManagement.repos[i];

  delete[] repoManagement.repos;
  repoManagement.repos = newRepos;
  repoManagement.numRepos++;
}

static void remove_repo_from_management(RepositoryManagement &repoManagement,
                                        Repository *repo) {
  if (repoManagement.numRepos == 0)
    return;

  Repository **newRepos =
      repoManagement.numRepos == 1 ? nullptr
                                   : new Repository *[repoManagement.numRepos - 1];
  int index = 0;

  for (int i = 0; i < repoManagement.numRepos; i++)
    if (repoManagement.repos[i] != repo)
      newRepos[index++] = repoManagement.repos[i];

  delete[] repoManagement.repos;
  repoManagement.repos = newRepos;
  repoManagement.numRepos--;
}

static void add_repo_to_user(User *user, Repository *repo) {
  Repository **newRepos =
      new Repository *[user->numRepos + 1];
  int insertIndex = 0;

  while (insertIndex < user->numRepos &&
         strcmp(user->repos[insertIndex]->name, repo->name) < 0) {
    newRepos[insertIndex] = user->repos[insertIndex];
    insertIndex++;
  }

  newRepos[insertIndex] = repo;

  for (int i = insertIndex; i < user->numRepos; i++)
    newRepos[i + 1] = user->repos[i];

  delete[] user->repos;
  user->repos = newRepos;
  user->numRepos++;
}

static void remove_repo_from_user(User *user, Repository *repo) {
  Repository **newRepos =
      user->numRepos <= 1 ? nullptr : new Repository *[user->numRepos - 1];
  int index = 0;

  for (int i = 0; i < user->numRepos; i++)
    if (user->repos[i] != repo)
      newRepos[index++] = user->repos[i];

  delete[] user->repos;
  user->repos = newRepos;
  user->numRepos--;
}

static void add_branch_to_repo(Repository *repo, Branch *branch) {
  Branch **newBranches = new Branch *[repo->numBranches + 1];
  for (int i = 0; i < repo->numBranches; i++)
    newBranches[i] = repo->branches[i];
  newBranches[repo->numBranches] = branch;

  delete[] repo->branches;
  repo->branches = newBranches;
  repo->numBranches++;
}

static void remove_branch_from_repo(Repository *repo, Branch *branch) {
  Branch **newBranches =
      repo->numBranches <= 1 ? nullptr : new Branch *[repo->numBranches - 1];
  int index = 0;

  for (int i = 0; i < repo->numBranches; i++)
    if (repo->branches[i] != branch)
      newBranches[index++] = repo->branches[i];

  delete[] repo->branches;
  repo->branches = newBranches;
  repo->numBranches--;
}

static void add_pr_to_repo(Repository *repo, PullRequest *pr) {
  PullRequest **newPrs = new PullRequest *[repo->numPrs + 1];
  for (int i = 0; i < repo->numPrs; i++)
    newPrs[i] = repo->prs[i];
  newPrs[repo->numPrs] = pr;
  delete[] repo->prs;
  repo->prs = newPrs;
  repo->numPrs++;
}

static void add_fork_to_repo(Repository *repo, Repository *forkRepo) {
  Repository **newForks = new Repository *[repo->numForks + 1];
  int insertIndex = 0;

  while (insertIndex < repo->numForks &&
         compare_repo_key(repo->forks[insertIndex]->owner->name,
                          repo->forks[insertIndex]->name,
                          forkRepo->owner->name, forkRepo->name) < 0) {
    newForks[insertIndex] = repo->forks[insertIndex];
    insertIndex++;
  }

  newForks[insertIndex] = forkRepo;

  for (int i = insertIndex; i < repo->numForks; i++)
    newForks[i + 1] = repo->forks[i];

  delete[] repo->forks;
  repo->forks = newForks;
  repo->numForks++;
}

static void remove_fork_from_repo(Repository *repo, Repository *forkRepo) {
  bool found = false;
  for (int i = 0; i < repo->numForks; i++)
    if (repo->forks[i] == forkRepo)
      found = true;

  if (!found)
    return;

  Repository **newForks =
      repo->numForks <= 1 ? nullptr : new Repository *[repo->numForks - 1];
  int index = 0;

  for (int i = 0; i < repo->numForks; i++)
    if (repo->forks[i] != forkRepo)
      newForks[index++] = repo->forks[i];

  delete[] repo->forks;
  repo->forks = newForks;
  repo->numForks--;
}

static void resort_forks(Repository *repo) {
  for (int i = 0; i < repo->numForks; i++)
    for (int j = i + 1; j < repo->numForks; j++)
      if (compare_repo_key(repo->forks[i]->owner->name, repo->forks[i]->name,
                           repo->forks[j]->owner->name,
                           repo->forks[j]->name) > 0) {
        Repository *tmp = repo->forks[i];
        repo->forks[i] = repo->forks[j];
        repo->forks[j] = tmp;
      }
}

static void parse_branch_fqn(const char *branchFQN, char *ownerName,
                             char *repoName, char *branchName,
                             bool &hasBranch) {
  ownerName[0] = '\0';
  repoName[0] = '\0';
  branchName[0] = '\0';
  hasBranch = false;

  char *buf = new char[strlen(branchFQN) + 1];
  strcpy(buf, branchFQN);

  char *ownerTok = strtok(buf, "/");
  char *repoTok = strtok(nullptr, ":");
  char *branchTok = strtok(nullptr, ":");

  if (ownerTok)
    strcpy(ownerName, ownerTok);
  if (repoTok)
    strcpy(repoName, repoTok);
  if (branchTok) {
    strcpy(branchName, branchTok);
    hasBranch = true;
  }

  delete[] buf;
}

static void delete_repository_object(Repository *repo) {
  delete[] repo->forks;

  for (int i = 0; i < repo->numBranches; i++) {
    delete_commit_chain(repo->branches[i]->head);
    delete repo->branches[i];
  }
  delete[] repo->branches;

  for (int i = 0; i < repo->numPrs; i++)
    delete repo->prs[i];
  delete[] repo->prs;

  delete_commit_chain(repo->commits);
  delete repo;
}

static int count_unique_source_commits(Commit *sourceHead, Commit *targetHead) {
  int count = 0;
  for (Commit *curr = sourceHead; curr; curr = curr->next) {
    bool found = false;
    for (Commit *target = targetHead; target; target = target->next)
      if (commits_equal(*curr, *target)) {
        found = true;
        break;
      }
    if (found)
      break;
    count++;
  }
  return count;
}

static void collect_unique_source_commits(Commit **arr, int count,
                                          Commit *sourceHead,
                                          Commit *targetHead) {
  int index = 0;
  for (Commit *curr = sourceHead; curr && index < count; curr = curr->next) {
    bool found = false;
    for (Commit *target = targetHead; target; target = target->next)
      if (commits_equal(*curr, *target)) {
        found = true;
        break;
      }
    if (found)
      break;
    arr[index++] = curr;
  }
}

static void delete_source_branch_after_merge(PullRequest *pr) {
  if (pr->fromBranch == nullptr)
    return;

  Repository *sourceRepo = pr->fromBranch->repo;
  if (!is_branch_in_repo(sourceRepo, pr->fromBranch))
    return;

  remove_branch_from_repo(sourceRepo, pr->fromBranch);
  delete_commit_chain(pr->fromBranch->head);
  delete pr->fromBranch;
  pr->fromBranch = nullptr;
}

/**
 * Task 1 - Register New User
 *
 * The `register_new_user` function registers a new user on the version control
 * system if no existing user has the specified username already.
 *
 * @param userManagement: reference to the `UserManagement` structure containing
 *                        the linked list of users.
 * @param username: the username of the new user to create.
 * @returns: `nullptr` if any validation failed; a pointer to the newly created
 *           user otherwise.
 */
const User *register_new_user(UserManagement &userManagement,
                              const char *username) {
  if (strlen(username) >= MAX_USER_NAME_LEN) {
    cout << "Username length exceeded " << MAX_USER_NAME_LEN - 1 << " characters." << endl;
    return nullptr;
  }

  for (User *curr = userManagement.head; curr; curr = curr->next)
    if (strcmp(curr->name, username) == 0) {
      cout << "User " << username << " already exists." << endl;
      return nullptr;
    }

  User *newUser = new User;
  strcpy(newUser->name, username);
  newUser->numRepos = 0;
  newUser->repos = nullptr;
  newUser->next = nullptr;

  if (!userManagement.head || strcmp(username, userManagement.head->name) < 0) {
    newUser->next = userManagement.head;
    userManagement.head = newUser;
  } else {
    User *prev = userManagement.head;
    while (prev->next && strcmp(username, prev->next->name) > 0) {
      prev = prev->next;
    }
    newUser->next = prev->next;
    prev->next = newUser;
  }

  return newUser;
}

/**
 * Task 2 - Create Repository
 *
 * The `create_repository` function creates a repository with the specified name
 * under the specified owner, if no existing repository under the very user has
 * the same name as specified.
 *
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param owner: the owner of the new repository.
 * @param repoName: the name of the new repository.
 * @param creationTimestamp: the timestamp for the creation of the repository,
 *                           used for the initial commit.
 * @returns: -1 if any validation fails before repository creation; the index
 *           into the array of pointers to repositories under the owner user
 *           associated with this new repository otherwise.
 */
int create_repository(RepositoryManagement &repoManagement, User *owner,
                      const char *repoName, const time_t creationTimestamp) {
  if (strlen(repoName) >= MAX_REPO_NAME_LEN) {
    cout << "Repository name length exceeded " << MAX_REPO_NAME_LEN - 1 << " characters." << endl;
    return -1;
  }

  char fqn[MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN + 2];
  sprintf(fqn, "%s/%s", owner->name, repoName);

  for (int i = 0; i < repoManagement.numRepos; i++) {
    if (strcmp(repoManagement.repos[i]->owner->name, owner->name) == 0 &&
        strcmp(repoManagement.repos[i]->name, repoName) == 0) {
      cout << "Repository " << fqn << " already exists." << endl;
      return -1;
    }
  }

  Commit *initCommit = new Commit;
  initCommit->author = owner;
  strcpy(initCommit->message, "Initial commit.");
  initCommit->timestamp = creationTimestamp;
  initCommit->next = nullptr;
  initCommit->prev = nullptr;
  compute_commit_hash(initCommit);

  Repository *newRepo = new Repository;
  newRepo->owner = owner;
  strcpy(newRepo->name, repoName);
  newRepo->numPrs = 0;
  newRepo->prs = nullptr;
  newRepo->numForks = 0;
  newRepo->forks = nullptr;
  newRepo->commits = initCommit;
  newRepo->numBranches = 0;
  newRepo->branches = nullptr;

  int insertIndex = 0;
  while (insertIndex < repoManagement.numRepos &&
         compare_repo_key(repoManagement.repos[insertIndex]->owner->name,
                          repoManagement.repos[insertIndex]->name,
                          owner->name, repoName) < 0)
    insertIndex++;

  insert_repo_into_management(repoManagement, newRepo);
  add_repo_to_user(owner, newRepo);

  return insertIndex;
}

/**
 * Task 3 - Create Branch
 *
 * The `create_branch` function creates a branch in the specified repository
 * with the supplied name and creator of the branch, at the specified commit,
 * if no existing branch has the same name already.
 *
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param repoFQN: the fully-qualified name of the repository to create a branch
 *                 for.
 * @param branchName: the name of the branch to create.
 * @param creator: pointer to the creator user of the branch
 * @param commit: the commit to create the branch from.
 * @returns: false if any validation failed; true if the branch was created
 *           successfully.
 */
bool create_branch(RepositoryManagement &repoManagement, char *repoFQN,
                   const char *branchName, const User *creator,
                   const Commit *commit) {
  if (strlen(branchName) >= MAX_BRANCH_NAME_LEN) {
    cout << "Branch name length exceeded " << MAX_BRANCH_NAME_LEN - 1 << " characters." << endl;
    return false;
  }

  Repository *repo = find_repository_by_fqn(repoManagement, repoFQN);
  if (!repo) {
    cout << "Repository " << repoFQN << " does not exist." << endl;
    return false;
  }

  if (find_branch(repo, branchName)) {
    char fullName[MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN + MAX_BRANCH_NAME_LEN + 3];
    sprintf(fullName, "%s:%s", repoFQN, branchName);
    cout << "Branch " << fullName << " already exists." << endl;
    return false;
  }

  const Commit *branchCommit = commit ? commit : repo->commits;
  Branch *newBranch = new Branch;
  strcpy(newBranch->name, branchName);
  newBranch->creator = creator;
  newBranch->head = clone_commit_chain(branchCommit);
  newBranch->repo = repo;

  add_branch_to_repo(repo, newBranch);
  return true;
}

/**
 * Task 4 - Add Commit
 *
 * The `add_commit` function adds a commit in the specified repository
 * with an optionally-specified branch to add the commit to. The hash
 * of the commit is computed from the author and message of the current
 * commit, as well as those of the previous commit (if any).
 *
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param author: the author of the commit.
 * @param repoFQN: fully-qualified name of the repository to add the commit
 *                 to.
 * @param commitMessage: the message of the commit.
 * @param branch: optionally, the branch the commit is added to.
 * @param timestamp: the timestamp when the commit was created.
 */
void add_commit(RepositoryManagement &repoManagement, const User *author,
                char *repoFQN, const char *branch, const char *commitMessage,
                time_t timestamp) {
  Repository *repo = find_repository_by_fqn(repoManagement, repoFQN);
  if (!repo) {
    cout << "Repository " << repoFQN << " does not exist." << endl;
    return;
  }

  Commit **headPtr = nullptr;
  char branchFQN[MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN + MAX_BRANCH_NAME_LEN + 3];

  if (branch != nullptr && strlen(branch) > 0) {
    Branch *targetBranch = find_branch(repo, branch);
    if (!targetBranch) {
      cout << "Branch " << repoFQN << ":" << branch << " does not exist." << endl;
      return;
    }
    headPtr = &targetBranch->head;
    sprintf(branchFQN, "%s:%s", repoFQN, branch);
  } else {
    headPtr = &repo->commits;
    sprintf(branchFQN, "%s:main", repoFQN);
  }

  char finalMsg[MAX_COMMIT_MSG_LEN];
  if (strlen(commitMessage) >= MAX_COMMIT_MSG_LEN) {
    cout << "Warning: commit message is longer than " << MAX_COMMIT_MSG_LEN - 1
         << " characters and will be truncated." << endl;
    strncpy(finalMsg, commitMessage, MAX_COMMIT_MSG_LEN - 1);
    finalMsg[MAX_COMMIT_MSG_LEN - 1] = '\0';
  } else {
    strcpy(finalMsg, commitMessage);
  }

  Commit *newCommit = new Commit;
  newCommit->author = author;
  strcpy(newCommit->message, finalMsg);
  newCommit->timestamp = timestamp;
  newCommit->prev = nullptr;
  newCommit->next = nullptr;

  Commit *curr = *headPtr;
  Commit *prev = nullptr;
  while (curr && curr->timestamp > timestamp) {
    prev = curr;
    curr = curr->next;
  }
  while (curr && curr->timestamp == timestamp) {
    prev = curr;
    curr = curr->next;
  }

  newCommit->prev = prev;
  newCommit->next = curr;
  if (prev)
    prev->next = newCommit;
  else
    *headPtr = newCommit;
  if (curr)
    curr->prev = newCommit;

  recompute_hashes(*headPtr);

  cout << "Pushing commit ";
  print_sha(newCommit->hash);
  cout << " to branch " << branchFQN << "." << endl;
}

/**
 * Task 5 - Transfer Ownership
 *
 * The `transfer_ownership` function transfers the ownership of a repository
 * from its current owner to another user. Both users have to be registered
 * users on the platform.
 *
 * @param userManagement: reference to the `UserManagement` structure containing
 *                        the linked list of users.
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param fromUsername: the name of the current owner of the specified
 *                      repsitory.
 * @param toUsername: the new owner of the specified repository.
 * @param repoName: the name of the repository.
 * @returns: true if the ownership transfer was successful; false otherwise.
 */
bool transfer_ownership(UserManagement &userManagement,
                        RepositoryManagement &repoManagement,
                        const char *fromUsername, const char *toUsername,
                        const char *repoName) {
  User *fromUser = find_user(userManagement, fromUsername);
  if (!fromUser) {
    cout << "User " << fromUsername << " does not exist." << endl;
    return false;
  }

  User *toUser = find_user(userManagement, toUsername);
  if (!toUser) {
    cout << "User " << toUsername << " does not exist." << endl;
    return false;
  }

  if (strcmp(fromUsername, toUsername) == 0) {
    cout << "Why are you transferring ownership to the same user?" << endl;
    return false;
  }

  Repository *targetRepo = find_repository(repoManagement, fromUsername, repoName);
  if (!targetRepo) {
    char fqn[MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN + 2];
    sprintf(fqn, "%s/%s", fromUsername, repoName);
    cout << "Repository " << fqn << " not found." << endl;
    return false;
  }

  if (find_repository(repoManagement, toUsername, repoName)) {
    char fqn[MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN + 2];
    sprintf(fqn, "%s/%s", toUsername, repoName);
    cout << "Repository " << fqn << " already exists." << endl;
    return false;
  }

  remove_repo_from_user(fromUser, targetRepo);
  remove_repo_from_management(repoManagement, targetRepo);

  targetRepo->owner = toUser;

  add_repo_to_user(toUser, targetRepo);
  insert_repo_into_management(repoManagement, targetRepo);

  for (int i = 0; i < repoManagement.numRepos; i++)
    resort_forks(repoManagement.repos[i]);
  return true;
}

bool create_pull_request(const RepositoryManagement &repoManagement,
                         const char *title, const User *author,
                         char *fromBranchFQN, char *toBranchFQN) {
  if (strlen(title) >= MAX_PR_TITLE_LEN) {
    cout << "Pull request title length exceeded " << MAX_PR_TITLE_LEN - 1
         << " characters." << endl;
    return false;
  }

  char fromOwner[MAX_USER_NAME_LEN];
  char fromRepoName[MAX_REPO_NAME_LEN];
  char fromBranchName[MAX_BRANCH_NAME_LEN];
  bool fromHasBranch;
  parse_branch_fqn(fromBranchFQN, fromOwner, fromRepoName, fromBranchName,
                   fromHasBranch);

  char toOwner[MAX_USER_NAME_LEN];
  char toRepoName[MAX_REPO_NAME_LEN];
  char toBranchName[MAX_BRANCH_NAME_LEN];
  bool toHasBranch;
  parse_branch_fqn(toBranchFQN, toOwner, toRepoName, toBranchName, toHasBranch);

  RepositoryManagement &mutableManagement =
      const_cast<RepositoryManagement &>(repoManagement);
  Repository *fromRepo =
      find_repository(mutableManagement, fromOwner, fromRepoName);
  Repository *toRepo = find_repository(mutableManagement, toOwner, toRepoName);

  if (!fromRepo) {
    cout << "Repository " << fromOwner << "/" << fromRepoName << " does not exist." << endl;
    return false;
  }
  if (!toRepo) {
    cout << "Repository " << toOwner << "/" << toRepoName << " does not exist." << endl;
    return false;
  }

  Branch *fromBranch = nullptr;
  if (fromHasBranch && strcmp(fromBranchName, "main") != 0) {
    fromBranch = find_branch(fromRepo, fromBranchName);
    if (!fromBranch) {
      cout << "Branch " << fromOwner << "/" << fromRepoName << ":" << fromBranchName
           << " does not exist." << endl;
      return false;
    }
  }

  Branch *toBranch = nullptr;
  if (toHasBranch && strcmp(toBranchName, "main") != 0) {
    toBranch = find_branch(toRepo, toBranchName);
    if (!toBranch) {
      cout << "Branch " << toOwner << "/" << toRepoName << ":" << toBranchName
           << " does not exist." << endl;
      return false;
    }
  }

  PullRequest *newPR = new PullRequest;
  newPR->author = author;
  newPR->id = toRepo->numPrs + 1;
  strcpy(newPR->title, title);
  newPR->repo = toRepo;
  newPR->fromBranch = fromBranch;
  newPR->toBranch = toBranch;
  newPR->status = OPEN;

  add_pr_to_repo(toRepo, newPR);

  cout << "Pull request #" << newPR->id << " has been created in "
       << toOwner << "/" << toRepoName << "." << endl;
  return true;
}

/**
 * Task 7 - Fork Repository
 *
 * The `fork_repository` function allows the creation of forks of repositories.
 *
 * @param userManagement: reference to the `UserManagement` structure containing
 *                        the linked list of users.
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param owner: the owner of the repository to create the fork from.
 * @param forkedOwner: the ownr of the forked repository.
 * @param repoToFork: the name of the repository to fork.
 * @returns true if the repository was forked successfully; false otherwise.
 */
bool fork_repository(UserManagement &userManagement,
                     RepositoryManagement &repoManagement, const User *owner,
                     const char *forkedOwner, const char *repoToFork) {
  Repository *sourceRepo =
      find_repository(repoManagement, owner->name, repoToFork);
  if (!sourceRepo) {
    cout << "Repository " << owner->name << "/" << repoToFork
         << " does not exist." << endl;
    return false;
  }

  User *newOwner = find_user(userManagement, forkedOwner);
  if (!newOwner) {
    cout << "User " << forkedOwner << " does not exist." << endl;
    return false;
  }

  if (find_repository(repoManagement, forkedOwner, repoToFork)) {
    cout << "Repository " << forkedOwner << "/" << repoToFork << " already exists." << endl;
    return false;
  }

  Repository *forkRepo = new Repository;
  forkRepo->owner = newOwner;
  strcpy(forkRepo->name, repoToFork);
  forkRepo->numPrs = 0;
  forkRepo->prs = nullptr;
  forkRepo->numForks = 0;
  forkRepo->forks = nullptr;
  forkRepo->numBranches = 0;
  forkRepo->branches = nullptr;
  forkRepo->commits = clone_commit_chain(sourceRepo->commits);

  for (int i = 0; i < sourceRepo->numBranches; i++) {
    Branch *srcBranch = sourceRepo->branches[i];
    Branch *newBranch = new Branch;
    strcpy(newBranch->name, srcBranch->name);
    newBranch->creator = srcBranch->creator;
    newBranch->repo = forkRepo;
    newBranch->head = clone_commit_chain(srcBranch->head);

    // Insert in alphabetical order
    Branch **newBranches = new Branch *[forkRepo->numBranches + 1];
    int insertindex = 0;
    while (insertindex < forkRepo->numBranches &&
           strcmp(forkRepo->branches[insertindex]->name, newBranch->name) < 0)
      insertindex++;
    for (int j = 0; j < insertindex; j++)
      newBranches[j] = forkRepo->branches[j];
    newBranches[insertindex] = newBranch;
    for (int j = insertindex; j < forkRepo->numBranches; j++)
      newBranches[j + 1] = forkRepo->branches[j];
    delete[] forkRepo->branches;
    forkRepo->branches = newBranches;
    forkRepo->numBranches++;
  }

  insert_repo_into_management(repoManagement, forkRepo);
  add_repo_to_user(newOwner, forkRepo);
  add_fork_to_repo(sourceRepo, forkRepo);

  cout << "Fork " << forkedOwner << "/" << repoToFork << " created successfully." << endl;
  return true;
}

/**
 * Task 8.1 - Merge Pull Request (Squash Merge)
 *
 * The `merge_pull_request_squashmerge` function merges the specified
 * pull request in a repsitory using the squash merge strategy
 * (combines all commits in the pull request into one and add it to the target
 * branch).
 *
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param repoFQN: the name of the repository to merge a pull request for.
 * @param prNumber: the number of the pull request to merge.
 * @param timestamp: the timestamp when this pull request was merged.
 */
void merge_pull_request_squashmerge(RepositoryManagement &repoManagement,
                                    char *repoFQN, int prNumber,
                                    time_t timestamp) {
  Repository *repo = find_repository_by_fqn(repoManagement, repoFQN);
  if (!repo) {
    cout << "Repository " << repoFQN << " does not exist." << endl;
    return;
  }

  if (prNumber > repo->numPrs || prNumber <= 0) {
    cout << "Invalid pull request number for repository " << repoFQN
         << ": out of range." << endl;
    return;
  }

  PullRequest *pr = repo->prs[prNumber - 1];
  Commit **targetHead = pr->toBranch ? &pr->toBranch->head : &repo->commits;

  Commit *newCommit = new Commit;
  newCommit->author = pr->author;
  sprintf(newCommit->message, "%s (#%d)", pr->title, pr->id);
  newCommit->timestamp = timestamp;
  newCommit->next = *targetHead;
  newCommit->prev = nullptr;

  if (newCommit->next)
    newCommit->next->prev = newCommit;

  *targetHead = newCommit;
  recompute_hashes(*targetHead);
  delete_source_branch_after_merge(pr);
  pr->status = MERGED;

  cout << "Pull request #" << prNumber << " in " << repoFQN
       << " has been merged using squash merge." << endl;
}

/**
 * Task 8.2 - Merge Pull Request (Rebase Merge)
 *
 * The `merge_pull_request_rebasemerge` function merges the specified
 * pull request in a repsitory using the rebase merge strategy
 * (rebases all commits in the pull request to the target branch).
 *
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param repoFQN: the name of the repository to merge a pull request for.
 * @param prNumber: the number of the pull request to merge.
 * @param timestamp: the timestamp when this pull request was merged.
 */
void merge_pull_request_rebasemerge(RepositoryManagement &repoManagement,
                                    char *repoFQN, int prNumber,
                                    time_t timestamp) {
  Repository *repo = find_repository_by_fqn(repoManagement, repoFQN);
  if (!repo) {
    cout << "Repository " << repoFQN << " does not exist." << endl;
    return;
  }

  if (prNumber > repo->numPrs || prNumber <= 0) {
    cout << "Invalid pull request number for repository " << repoFQN
         << ": out of range." << endl;
    return;
  }

  PullRequest *pr = repo->prs[prNumber - 1];
  Commit *sourceHead = pr->fromBranch ? pr->fromBranch->head : repo->commits;
  Commit **targetHeadPtr = pr->toBranch ? &pr->toBranch->head : &repo->commits;
  Commit *targetHead = *targetHeadPtr;

  const int sourceUniqueCount = count_unique_source_commits(sourceHead, targetHead);
  Commit **sourceUnique = sourceUniqueCount == 0 ? nullptr : new Commit *[sourceUniqueCount];
  collect_unique_source_commits(sourceUnique, sourceUniqueCount, sourceHead, targetHead);

  for (int i = sourceUniqueCount - 1; i >= 0; i--) {
    Commit *newCommit = new Commit;
    newCommit->author = sourceUnique[i]->author;
    strcpy(newCommit->message, sourceUnique[i]->message);
    newCommit->timestamp = timestamp;
    newCommit->prev = nullptr;
    newCommit->next = *targetHeadPtr;

    if (*targetHeadPtr)
      (*targetHeadPtr)->prev = newCommit;
    *targetHeadPtr = newCommit;
  }

  delete[] sourceUnique;
  recompute_hashes(*targetHeadPtr);
  delete_source_branch_after_merge(pr);
  pr->status = MERGED;

  cout << "Pull request #" << prNumber << " in " << repoFQN
       << " has been merged using rebase merge." << endl;
}

/**
 * Task 8.3 - Merge Pull Request (Merge Commit)
 *
 * The `merge_pull_request_mergecommit` function merges the specified
 * pull request in a repsitory using the merge commit strategy
 * (adds all commits to the target branch preserving chronological order, with a
 * final merge commit added).
 *
 * This is slightly different from what actually happens when a merge commit
 * is used, but for simplicity's sake this is done instead.
 *
 * @param repoManagement: reference to the `RepositoryManagement` structure
 *                        containing the dynamic array of pointers to
 *                        repositories.
 * @param repoFQN: the name of the repository to merge a pull request for.
 * @param prNumber: the number of the pull request to merge.
 * @param timestamp: the timestamp when this pull request was merged.
 */
void merge_pull_request_mergecommit(RepositoryManagement &repoManagement,
                                    char *repoFQN, int prNumber,
                                    time_t timestamp) {
  Repository *repo = find_repository_by_fqn(repoManagement, repoFQN);
  if (!repo) {
    cout << "Repository " << repoFQN << " does not exist." << endl;
    return;
  }

  if (prNumber > repo->numPrs || prNumber <= 0) {
    cout << "Invalid pull request number for repository " << repoFQN
         << ": out of range." << endl;
    return;
  }

  PullRequest *pr = repo->prs[prNumber - 1];
  Commit *sourceHead = pr->fromBranch ? pr->fromBranch->head : repo->commits;
  Commit **targetHeadPtr = pr->toBranch ? &pr->toBranch->head : &repo->commits;
  Commit *targetHead = *targetHeadPtr;

  const int sourceUniqueCount = count_unique_source_commits(sourceHead, targetHead);
  Commit **sourceUnique = sourceUniqueCount == 0 ? nullptr : new Commit *[sourceUniqueCount];
  collect_unique_source_commits(sourceUnique, sourceUniqueCount, sourceHead, targetHead);

  int targetCount = 0;
  for (Commit *curr = targetHead; curr; curr = curr->next)
    targetCount++;

  Commit **targetArr = targetCount == 0 ? nullptr : new Commit *[targetCount];
  int targetindex = 0;
  for (Commit *curr = targetHead; curr; curr = curr->next)
    targetArr[targetindex++] = curr;

  Commit **sourceCopies =
      sourceUniqueCount == 0 ? nullptr : new Commit *[sourceUniqueCount];
  for (int i = 0; i < sourceUniqueCount; i++) {
    sourceCopies[i] = new Commit;
    sourceCopies[i]->author = sourceUnique[i]->author;
    strcpy(sourceCopies[i]->message, sourceUnique[i]->message);
    sourceCopies[i]->timestamp = sourceUnique[i]->timestamp;
    sourceCopies[i]->prev = nullptr;
    sourceCopies[i]->next = nullptr;
  }

  Commit **merged =
      targetCount + sourceUniqueCount == 0
          ? nullptr
          : new Commit *[targetCount + sourceUniqueCount];
  int i = 0, j = 0, mergedCount = 0;
  while (i < sourceUniqueCount && j < targetCount) {
    if (sourceCopies[i]->timestamp >= targetArr[j]->timestamp)
      merged[mergedCount++] = sourceCopies[i++];
    else
      merged[mergedCount++] = targetArr[j++];
  }
  while (i < sourceUniqueCount)
    merged[mergedCount++] = sourceCopies[i++];
  while (j < targetCount)
    merged[mergedCount++] = targetArr[j++];

  for (int k = 0; k < mergedCount; k++) {
    merged[k]->prev = k == 0 ? nullptr : merged[k - 1];
    merged[k]->next = k + 1 < mergedCount ? merged[k + 1] : nullptr;
  }

  Commit *mergedHead = mergedCount == 0 ? nullptr : merged[0];

  Commit *mergeCommit = new Commit;
  mergeCommit->author = pr->author;
  char sourceBranchFQN[MAX_USER_NAME_LEN + MAX_REPO_NAME_LEN + MAX_BRANCH_NAME_LEN + 8];
  if (pr->fromBranch)
    sprintf(sourceBranchFQN, "%s/%s:%s", pr->fromBranch->repo->owner->name,
            pr->fromBranch->repo->name, pr->fromBranch->name);
  else
    sprintf(sourceBranchFQN, "%s:main", repoFQN);
  snprintf(mergeCommit->message, MAX_COMMIT_MSG_LEN, "Merge pull request #%d from branch %s",
           pr->id, sourceBranchFQN);
  mergeCommit->timestamp = timestamp;
  mergeCommit->next = mergedHead;
  mergeCommit->prev = nullptr;

  if (mergedHead)
    mergedHead->prev = mergeCommit;

  *targetHeadPtr = mergeCommit;
  recompute_hashes(*targetHeadPtr);

  delete[] merged;
  delete[] sourceCopies;
  delete[] sourceUnique;
  delete[] targetArr;

  delete_source_branch_after_merge(pr);
  pr->status = MERGED;

  cout << "Pull request #" << prNumber << " in " << repoFQN
       << " has been merged using a merge commit." << endl;
}

void deregister_user(UserManagement &userManagement,
                     RepositoryManagement &repoManagement,
                     const char *username) {
  
  // Find the target user and their predecessor
  User *targetUser = nullptr;
  User *previousUser = nullptr;
  User *currentUser = userManagement.head;
  
  while (currentUser != nullptr) {
    if (strcmp(currentUser->name, username) == 0) {
      targetUser = currentUser;
      break;
    }
    previousUser = currentUser;
    currentUser = currentUser->next;
  }
  
  // Validate user existence
  if (targetUser == nullptr) {
    cout << "User " << username << " does not exist." << endl;
    return;
  }
  
  // Replace all references to targetUser with ghost user across repositories
  for (int repoIndex = 0; repoIndex < repoManagement.numRepos; ++repoIndex) {
    Repository *currentRepo = repoManagement.repos[repoIndex];
    
    // Skip repositories owned by the target user
    if (currentRepo->owner == targetUser) {
      continue;
    }
    
    // Update commit authors in the main commit chain
    for (Commit *commitPtr = currentRepo->commits; commitPtr != nullptr; commitPtr = commitPtr->next) {
      if (commitPtr->author == targetUser) {
        commitPtr->author = ghost;
      }
    }
    
    // Update branch creators and their commit authors
    for (int branchIdx = 0; branchIdx < currentRepo->numBranches; ++branchIdx) {
      Branch *currentBranch = currentRepo->branches[branchIdx];
      
      if (currentBranch->creator == targetUser) {
        currentBranch->creator = ghost;
      }
      
      for (Commit *commitPtr = currentBranch->head; commitPtr != nullptr; commitPtr = commitPtr->next) {
        if (commitPtr->author == targetUser) {
          commitPtr->author = ghost;
        }
      }
    }
    
    // Update pull request authors
    for (int prIdx = 0; prIdx < currentRepo->numPrs; ++prIdx) {
      if (currentRepo->prs[prIdx]->author == targetUser) {
        currentRepo->prs[prIdx]->author = ghost;
      }
    }
  }
  
  // Delete all repositories owned by the target user
  while (targetUser->numRepos > 0) {
    Repository *ownedRepo = targetUser->repos[0];
    
    // Remove this repository from all forks
    for (int mgmtIdx = 0; mgmtIdx < repoManagement.numRepos; ++mgmtIdx) {
      remove_fork_from_repo(repoManagement.repos[mgmtIdx], ownedRepo);
    }
    
    // Remove from management system and user's list, then delete
    remove_repo_from_management(repoManagement, ownedRepo);
    remove_repo_from_user(targetUser, ownedRepo);
    delete_repository_object(ownedRepo);
  }
  
  // Clean up user's repository array
  delete[] targetUser->repos;
  
  // Remove user from linked list
  if (previousUser != nullptr) {
    previousUser->next = targetUser->next;
  } else {
    userManagement.head = targetUser->next;
  }
  
  // Free the user memory
  delete targetUser;
  
  cout << "User " << username << " has been deregistered." << endl;
}