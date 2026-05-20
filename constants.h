#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

// ====    Region: Constants ====

/**
 * The maximum length of the name of a branch.
 */
#define MAX_BRANCH_NAME_LEN 16

/**
 * The maxmimum length of the message of a commit.
 */
#define MAX_COMMIT_MSG_LEN 64

/**
 * The maximum length of the title of a pull reuqest.
 */
#define MAX_PR_TITLE_LEN 32

/**
 * The maximum length of the name of a repository.
 */
#define MAX_REPO_NAME_LEN 32

/**
 * The maximum length of the name of a user on the system.
 */
#define MAX_USER_NAME_LEN 32

// ==== Endregion: Constants ====

// ====    Region: Enumerations ====

/**
 * The status of a pull request in a repository.
 */
enum PullRequestStatus {
  OPEN,
  CLOSED,
  MERGED,
};

/**
 * The strategy of which how a pull request should be merged.
 *
 * - Merge Commit    : a merge commit will be created at the end after replaying
 *                     all commits from a pull request on the target branch.
 * - Rebase Merge    : all commits of the pull request will be rebased on top
 *                     of the target branch.
 * - Squash and Merge: the commits from the pull request will be squashed into
 *                     one commit and added to the target branch.
 */
enum PullRequestMergeStrategy {
  MERGE_COMMIT,
  REBASE_MERGE,
  SQUASH_AND_MERGE,
};

// ==== Endregion: Enumerations ====

#endif /* __CONSTANTS_H__ */