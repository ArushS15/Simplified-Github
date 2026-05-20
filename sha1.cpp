#include <climits>
#include <cstdint>
#include <cstring>
#include <iomanip>

#include "sha1.h"

void initialize(SHA1 &sha) {
  // FIX: must zero-init state fields (not just buf), otherwise input() reads garbage
  sha.loLen = sha.hiLen = sha.msgBlockIndex = 0;
  sha.corrupted = sha.computed = false;

  sha.buf[0] = 0x67452301;
  sha.buf[1] = 0xEFCDAB89;
  sha.buf[2] = 0x98BADCFE;
  sha.buf[3] = 0x10325476;
  sha.buf[4] = 0xC3D2E1F0;
}

bool digest(SHA1 &sha) {
  if (sha.corrupted)
    return false;

  if (!sha.computed) {
    pad_message(sha);
    sha.computed = true;
  }

  return true;
}

void input(SHA1 &sha, const char *msg, int len) {
  if (!len)
    return;

  if (sha.computed || sha.corrupted) {
    sha.corrupted = true;
    return;
  }

  while (len-- && !sha.corrupted) {
    sha.msgBlock[sha.msgBlockIndex++] = *msg & 0xFF;

    sha.loLen += 8;
    sha.loLen &= 0xFFFFFFFF;

    if (sha.loLen == 0) {
      sha.hiLen++;
      sha.hiLen &= 0xFFFFFFFF;

      if (sha.hiLen == 0)
        sha.corrupted = true;
    }

    if (sha.msgBlockIndex == 64)
      process_message_block(sha);

    msg++;
  }
}

void input(SHA1 &sha, time_t timestamp) {
  uint64_t val = 0;
  memcpy(&val, &timestamp, sizeof(timestamp));
  
  for (int i = 7; i >= 0; i--) {
    uint8_t byte = static_cast<uint8_t>((val >> (i * 8)) & 0xFF);
    input(sha, reinterpret_cast<char*>(&byte), 1);
  }
}

void reset(SHA1 &sha) {
  sha.loLen = sha.hiLen = sha.msgBlockIndex = 0;
  sha.corrupted = sha.computed = false;

  sha.buf[0] = 0x67452301;
  sha.buf[1] = 0xEFCDAB89;
  sha.buf[2] = 0x98BADCFE;
  sha.buf[3] = 0x10325476;
  sha.buf[4] = 0xC3D2E1F0;
}

bool hashes_equal(const SHA1 &self, const SHA1 &that) {
  for (int i = 0; i < 5; i++) {
    if (self.buf[i] != that.buf[i])
      return false;
  }

  return true;
}

void sha1_from_hash_str(SHA1 &sha, const char *hash) {
  if (strlen(hash) != 40) {
    cout << "Invalid SHA1 hash: must be 20 bytes in length" << endl;
    return;
  }

  for (int i = 0; i < 5; i++) {
    unsigned int w = 0;

    for (int j = 0; j < 8; j++) {
      char c = hash[i * 8 + j];
      unsigned int v = (c >= '0' && c <= '9')   ? c - '0'
                       : (c >= 'a' && c <= 'f') ? c - 'a' + 10
                       : (c >= 'A' && c <= 'F') ? c - 'A' + 10
                                                : INT_MAX;
      if (v == INT_MAX) {
        std::cout << "Hash contains invalid digit, expected hexadecimal"
                  << endl;
        return;
      }

      w = (w << 4) | v;
    }

    sha.buf[i] = w;
  }
}

void print_sha(const SHA1 &sha) {
  cout << hex << setfill('0') << setw(8) << sha.buf[0] << setw(8) << sha.buf[1]
       << setw(8) << sha.buf[2] << setw(8) << sha.buf[3] << setw(8)
       << sha.buf[4];
}

unsigned int circular_shift(unsigned int word, int shamt) {
  return ((word << shamt) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32 - shamt));
}

void pad_message(SHA1 &sha) {
  if (sha.msgBlockIndex > 55) {
    sha.msgBlock[sha.msgBlockIndex++] = 0x80;

    while (sha.msgBlockIndex < 64)
      sha.msgBlock[sha.msgBlockIndex++] = 0;

    process_message_block(sha);

    while (sha.msgBlockIndex < 56)
      sha.msgBlock[sha.msgBlockIndex++] = 0;
  } else {
    sha.msgBlock[sha.msgBlockIndex++] = 0x80;

    while (sha.msgBlockIndex < 56)
      sha.msgBlock[sha.msgBlockIndex++] = 0;
  }

  sha.msgBlock[56] = (sha.hiLen >> 24) & 0xFF;
  sha.msgBlock[57] = (sha.hiLen >> 16) & 0xFF;
  sha.msgBlock[58] = (sha.hiLen >> 8) & 0xFF;
  sha.msgBlock[59] = sha.hiLen & 0xFF;
  sha.msgBlock[60] = (sha.loLen >> 24) & 0xFF;
  sha.msgBlock[61] = (sha.loLen >> 16) & 0xFF;
  sha.msgBlock[62] = (sha.loLen >> 8) & 0xFF;
  sha.msgBlock[63] = sha.loLen & 0xFF;

  process_message_block(sha);
}

void process_message_block(SHA1 &sha) {
  const unsigned int K[4] = {0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6};
  unsigned int W[80];

  for (int i = 0; i < 16; i++) {
    W[i] = static_cast<unsigned int>(sha.msgBlock[i * 4]) << 24;
    W[i] |= static_cast<unsigned int>(sha.msgBlock[i * 4 + 1]) << 16;
    W[i] |= static_cast<unsigned int>(sha.msgBlock[i * 4 + 2]) << 8;
    W[i] |= static_cast<unsigned int>(sha.msgBlock[i * 4 + 3]);
  }

  for (int i = 16; i < 80; i++)
    W[i] = circular_shift(W[i - 3] ^ W[i - 8] ^ W[i - 14] ^ W[i - 16], 1);

  unsigned int A = sha.buf[0];
  unsigned int B = sha.buf[1];
  unsigned int C = sha.buf[2];
  unsigned int D = sha.buf[3];
  unsigned int E = sha.buf[4];

  unsigned int temp;

  for (int i = 0; i < 20; i++) {
    temp = circular_shift(A, 5) + ((B & C) | ((~B) & D)) + E + W[i] + K[0];
    temp &= 0xFFFFFFFF;

    E = D;
    D = C;
    C = circular_shift(B, 30);
    B = A;
    A = temp;
  }

  for (int i = 20; i < 40; i++) {
    temp = circular_shift(A, 5) + (B ^ C ^ D) + E + W[i] + K[1];
    temp &= 0xFFFFFFFF;

    E = D;
    D = C;
    C = circular_shift(B, 30);
    B = A;
    A = temp;
  }

  for (int i = 40; i < 60; i++) {
    temp =
        circular_shift(A, 5) + ((B & C) | (B & D) | (C & D)) + E + W[i] + K[2];
    temp &= 0xFFFFFFFF;

    E = D;
    D = C;
    C = circular_shift(B, 30);
    B = A;
    A = temp;
  }

  for (int i = 60; i < 80; i++) {
    temp = circular_shift(A, 5) + (B ^ C ^ D) + E + W[i] + K[3];
    temp &= 0xFFFFFFFF;

    E = D;
    D = C;
    C = circular_shift(B, 30);
    B = A;
    A = temp;
  }

  sha.buf[0] = (sha.buf[0] + A) & 0xFFFFFFFF;
  sha.buf[1] = (sha.buf[1] + B) & 0xFFFFFFFF;
  sha.buf[2] = (sha.buf[2] + C) & 0xFFFFFFFF;
  sha.buf[3] = (sha.buf[3] + D) & 0xFFFFFFFF;
  sha.buf[4] = (sha.buf[4] + E) & 0xFFFFFFFF;

  sha.msgBlockIndex = 0;
}
