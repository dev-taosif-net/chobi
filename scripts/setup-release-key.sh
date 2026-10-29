#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# One-time setup of the key that signs update manifests (run by the project owner).
#
#   1. builds tools/chobi_sign.cpp,
#   2. creates a new Ed25519 key in ~/.config/chobi-release/ (never inside the repo),
#   3. stores the secret seed as the GitHub Actions secret CHOBI_UPDATE_SIGNING_SEED,
#   4. writes the public key to packaging/update-signing-key.pub (commit this file).
#
# The secret is never printed. Back up ~/.config/chobi-release/update-signing-seed.txt
# somewhere safe and offline: installed copies of Chobi only accept updates signed by it.
set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
key_dir="${CHOBI_RELEASE_KEY_DIR:-$HOME/.config/chobi-release}"
seed_file="$key_dir/update-signing-seed.txt"
pub_file="$repo_root/packaging/update-signing-key.pub"
github_repo="${CHOBI_GITHUB_REPO:-dev-taosif-net/chobi}"

command -v gh >/dev/null || { echo "needs the GitHub CLI (gh), logged in" >&2; exit 1; }

work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mc="$repo_root/third_party/monocypher"
cc -O2 -c "$mc/monocypher.c" -o "$work/mc.o"
cc -O2 -I"$mc" -c "$mc/monocypher-ed25519.c" -o "$work/mce.o"
c++ -std=c++17 -O2 -I"$mc" "$repo_root/tools/chobi_sign.cpp" "$work/mc.o" "$work/mce.o" \
    -o "$work/chobi-sign"

mkdir -p "$key_dir"
chmod 700 "$key_dir"
if [[ -f "$seed_file" ]]; then
  echo "Using the existing key in $seed_file"
  # Recompute the public key from the existing seed.
  printf 'probe' > "$work/probe"
  CHOBI_UPDATE_SIGNING_SEED="$(cat "$seed_file")" "$work/chobi-sign" sign "$work/probe" \
    | sed -n 's/.*public key //p' > "$work/pub"
else
  (umask 077; "$work/chobi-sign" keygen > "$work/keygen")
  sed -n 's/^seed[^:]*: //p' "$work/keygen" > "$seed_file"
  sed -n 's/^public[^:]*: //p' "$work/keygen" > "$work/pub"
  chmod 600 "$seed_file"
  rm -f "$work/keygen"
  echo "Created a new signing key in $seed_file"
fi

gh secret set CHOBI_UPDATE_SIGNING_SEED -R "$github_repo" < "$seed_file"
cp "$work/pub" "$pub_file"
echo
echo "Done. Public key: $(cat "$pub_file")"
echo "Next:"
echo "  git add packaging/update-signing-key.pub && git commit -m 'Add update signing public key' && git push"
echo "  Back up $seed_file offline (losing it means users must reinstall manually once)."
