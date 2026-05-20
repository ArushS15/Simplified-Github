#ifndef __PA3_TASK_H__
#define __PA3_TASK_H__

#include "structures.h"

extern User *ghost;

const User *register_new_user(UserManagement &userManagement,
                              const char *username);

int create_repository(RepositoryManagement &repoManagement, User *owner,
                      const char *repoName, time_t creationTimestamp);

bool create_branch(RepositoryManagement &repoManagement, char *repoFQN,
                   const char *branchName, const User *creator,
                   const Commit *commit);

void add_commit(RepositoryManagement &repoManagement, const User *author,
                char *repoFQN, const char *branch, const char *commitMessage,
                time_t timestamp);

bool transfer_ownership(UserManagement &userManagement,
                        RepositoryManagement &repoManagement,
                        const char *fromUsername, const char *toUsername,
                        const char *repoName);

bool create_pull_request(const RepositoryManagement &repoManagement,
                         const char *title, const User *author,
                         char *fromBranchFQN, char *toBranchFQN);

bool fork_repository(UserManagement &userManagement,
                     RepositoryManagement &repoManagement,
                     const User *originalOwner, const char *forkedOwner,
                     const char *repoToFork);

void merge_pull_request_mergecommit(RepositoryManagement &repoManagement,
                                    char *repoName, int prNumber,
                                    time_t timestamp);

void merge_pull_request_rebasemerge(RepositoryManagement &repoManagement,
                                    char *repoName, int prNumber,
                                    time_t timestamp);

void merge_pull_request_squashmerge(RepositoryManagement &repoManagement,
                                    char *repoName, int prNumber,
                                    time_t timestamp);

void deregister_user(UserManagement &userManagement,
                     RepositoryManagement &repoManagement,
                     const char *username);

#endif /* __PA3_TASK_H__ */
