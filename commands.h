#ifndef __COMMANDS_H__
#define __COMMANDS_H__

#include <map>
#include <string>
#include <vector>

#include "structures.h"

using namespace std;

typedef struct {
  const char *params, *desc;
  size_t nParams;
  void (*f)(const vector<string>&);
} command_t;

extern multimap<string, command_t> commands;

extern RepositoryManagement repoManagement;
extern UserManagement userManagement;

#endif
