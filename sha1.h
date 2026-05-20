#ifndef __SHA1_H__
#define __SHA1_H__

#include <ctime>
#include <iostream>

using namespace std;

struct SHA1 {
  unsigned int buf[5];
  unsigned int loLen;
  unsigned int hiLen;

  unsigned char msgBlock[64];
  int msgBlockIndex;
  bool computed;
  bool corrupted;
};

void initialize(SHA1 &sha);
bool digest(SHA1 &sha);

void input(SHA1 &sha, const char *msg, int len);
void input(SHA1 &sha, time_t timestamp);

void reset(SHA1 &sha);

bool hashes_equal(const SHA1 &left, const SHA1 &right);

void sha1_from_hash_str(SHA1 &sha, const char *hash);
void print_sha(const SHA1 &sha);

void pad_message(SHA1 &sha);
void process_message_block(SHA1 &sha);

unsigned int circular_shift(unsigned int word, int shamt);

#endif /* __SHA1_H__ */
