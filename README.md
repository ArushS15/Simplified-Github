## Simplified Version Control System

A terminal-based version control system (VCS) implementation in C++ that mimics core GitHub-like functionality, including user management, repositories, branches, commits, pull requests, forks, and merge strategies.

## 🚀 Features

### User Management
- Register new users with unique usernames
- De-register users (replaces references with a "ghost" user)
- Maintains a lexicographically sorted linked list of users

### Repository Operations
- Create repositories with unique names per user
- Transfer repository ownership between users
- Fork repositories (deep copies all commits and branches)
- Repository management with sorted dynamic arrays

### Branching
- Create branches from any commit
- Each branch has its own independent commit history
- Branches sorted lexicographically by name

### Commits
- Add commits to the main branch or specific branches
- Automatic SHA-1 hash computation based on author, message, and previous commit metadata
- Commit messages are truncated if too long
- Chronologically sorted commit history

### Pull Requests
- Create pull requests between branches (same or different repositories)
- Three merge strategies:
  - **Squash Merge**: Combines all commits into a single commit
  - **Rebase Merge**: Rebases commits onto target branch
  - **Merge Commit**: Preserves chronological order with a merge commit
- Automatic cleanup of the source branch after merge

### Data Structures
- **Users**: Singly linked list (sorted by username)
- **Repositories**: Dynamic array (sorted by owner + repository name)
- **Commits**: Doubly linked list (sorted by timestamp)
- **Branches**: Dynamic array (sorted by name)
- **Pull Requests**: Dynamic array (sorted by ID)
- **Forks**: Dynamic array (sorted by owner + repository name)

## 🛠️ Technical Details

### Language & Tools
- **Language**: C++17
- **Hash Algorithm**: SHA-1 (provided implementation)
- **Memory Management**: Manual dynamic allocation/deallocation
### Compilation
```bash
make
