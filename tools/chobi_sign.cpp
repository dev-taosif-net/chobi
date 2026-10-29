// SPDX-License-Identifier: GPL-3.0-or-later
// Release signing tool for update manifests (used by .github/workflows/release.yml).
//
//   chobi-sign keygen               print a new seed (keep secret!) and its public key
//   chobi-sign sign FILE            sign FILE with $CHOBI_UPDATE_SIGNING_SEED -> FILE.sig
//   chobi-sign verify FILE PUBKEY   check FILE.sig against a base64 public key (exit 0 = valid)
//
// Keys are standard Ed25519 (RFC 8032): the secret is a 32-byte seed, all values are base64.
// Depends only on the vendored Monocypher, so it builds without Qt.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

extern "C" {
#include "monocypher-ed25519.h"
}

namespace {

const char kAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string base64Encode(const unsigned char* data, std::size_t size) {
  std::string out;
  for (std::size_t i = 0; i < size; i += 3) {
    const unsigned v = (data[i] << 16) | (i + 1 < size ? data[i + 1] << 8 : 0) |
                       (i + 2 < size ? data[i + 2] : 0);
    out += kAlphabet[(v >> 18) & 63];
    out += kAlphabet[(v >> 12) & 63];
    out += i + 1 < size ? kAlphabet[(v >> 6) & 63] : '=';
    out += i + 2 < size ? kAlphabet[v & 63] : '=';
  }
  return out;
}

bool base64Decode(const std::string& text, std::vector<unsigned char>* out) {
  out->clear();
  unsigned buffer = 0;
  int bits = 0;
  for (char c : text) {
    if (c == '=' || c == '\n' || c == '\r' || c == ' ') continue;
    const char* p = std::strchr(kAlphabet, c);
    if (!p || c == '\0') return false;
    buffer = (buffer << 6) | static_cast<unsigned>(p - kAlphabet);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      out->push_back(static_cast<unsigned char>((buffer >> bits) & 0xFF));
    }
  }
  return true;
}

bool readFile(const std::string& path, std::vector<unsigned char>* out) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return false;
  out->assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  return true;
}

int keygen() {
  unsigned char seed[32];
  FILE* random = std::fopen("/dev/urandom", "rb");
  if (!random || std::fread(seed, 1, sizeof seed, random) != sizeof seed) {
    std::fprintf(stderr, "cannot read /dev/urandom\n");
    return 1;
  }
  std::fclose(random);
  const std::string seedText = base64Encode(seed, sizeof seed);
  unsigned char secret[64], pub[32];
  crypto_ed25519_key_pair(secret, pub, seed);  // wipes `seed`
  std::printf("seed (SECRET, store as CHOBI_UPDATE_SIGNING_SEED): %s\n", seedText.c_str());
  std::printf("public key (embed in CMakeLists.txt): %s\n", base64Encode(pub, sizeof pub).c_str());
  crypto_wipe(secret, sizeof secret);
  return 0;
}

int sign(const std::string& path) {
  const char* env = std::getenv("CHOBI_UPDATE_SIGNING_SEED");
  std::vector<unsigned char> seed, message;
  if (!env || !base64Decode(env, &seed) || seed.size() != 32) {
    std::fprintf(stderr, "CHOBI_UPDATE_SIGNING_SEED must hold a base64 32-byte seed\n");
    return 1;
  }
  if (!readFile(path, &message)) {
    std::fprintf(stderr, "cannot read %s\n", path.c_str());
    return 1;
  }
  unsigned char secret[64], pub[32], sig[64];
  crypto_ed25519_key_pair(secret, pub, seed.data());
  crypto_ed25519_sign(sig, secret, message.data(), message.size());
  crypto_wipe(secret, sizeof secret);

  std::ofstream out(path + ".sig", std::ios::binary);
  out << base64Encode(sig, sizeof sig) << '\n';
  std::printf("signed %s with public key %s\n", path.c_str(), base64Encode(pub, sizeof pub).c_str());
  return out ? 0 : 1;
}

int verify(const std::string& path, const std::string& pubText) {
  std::vector<unsigned char> message, sigFile, sig, pub;
  if (!readFile(path, &message) || !readFile(path + ".sig", &sigFile)) {
    std::fprintf(stderr, "cannot read %s or its .sig\n", path.c_str());
    return 1;
  }
  if (!base64Decode(std::string(sigFile.begin(), sigFile.end()), &sig) || sig.size() != 64 ||
      !base64Decode(pubText, &pub) || pub.size() != 32) {
    std::fprintf(stderr, "malformed signature or public key\n");
    return 1;
  }
  const bool ok = crypto_ed25519_check(sig.data(), pub.data(), message.data(), message.size()) == 0;
  std::printf("%s: %s\n", path.c_str(), ok ? "signature OK" : "SIGNATURE INVALID");
  return ok ? 0 : 2;
}

}  // namespace

int main(int argc, char** argv) {
  const std::string cmd = argc > 1 ? argv[1] : "";
  if (cmd == "keygen" && argc == 2) return keygen();
  if (cmd == "sign" && argc == 3) return sign(argv[2]);
  if (cmd == "verify" && argc == 4) return verify(argv[2], argv[3]);
  std::fprintf(stderr, "usage: chobi-sign keygen | sign FILE | verify FILE PUBKEY\n");
  return 64;
}
