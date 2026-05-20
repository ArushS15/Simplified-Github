#ifndef __GIVEN_H__
#define __GIVEN_H__

#include <ctime>
#include <iostream>

#include "structures.h"

using namespace std;

// ====    Region: Given Functions ====

void merge_pull_request(RepositoryManagement &repoManagement, char *repoName,
                        int prNumber, time_t timestamp,
                        PullRequestMergeStrategy strategy);

// ==== Endregion: Given Functions ====

// ====    Region: Print Functions ====

void print(const Branch &Branch, bool details = false);
void print(Commit &commit);
void print(const PullRequest &pr, bool details = false);
void print(const Repository &repo, bool details = false);
void print(const User &user, bool details = false);

// ==== Endregion: Print Functions ====

// ====    Region: Equality Functions ====

bool commits_equal(const Commit &l, const Commit &r);

// ==== Endregion: Equality Functions ====

#endif /* __GIVEN_H__ */
