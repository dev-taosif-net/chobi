#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Cuts a release:  scripts/release.sh 0.3.0
#
#   - checks you're on an up-to-date, clean main with an "Unreleased" changelog section,
#   - sets the version in CMakeLists.txt and the AppStream metainfo,
#   - turns "## [Unreleased]" into "## [0.3.0] - <date>" (keeping a fresh Unreleased section),
#   - runs the unit tests, commits "Release 0.3.0", tags v0.3.0 and pushes both.
#
# The tag starts .github/workflows/release.yml, which builds the AppImage and the Windows
# installer, signs update.json and publishes the GitHub release. Installed copies then offer
# the update within a day (or immediately via Settings → Check now).
set -euo pipefail

version="${1:?usage: scripts/release.sh X.Y.Z}"
[[ "$version" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "expected X.Y.Z, got: $version" >&2; exit 1; }
cd "$(dirname "$0")/.."

[[ -s packaging/update-signing-key.pub ]] || {
  echo "No update signing key yet: run scripts/setup-release-key.sh first." >&2; exit 1; }
[[ "$(git rev-parse --abbrev-ref HEAD)" == main ]] || { echo "switch to main first" >&2; exit 1; }
[[ -z "$(git status --porcelain)" ]] || { echo "working tree not clean" >&2; exit 1; }
git fetch -q origin main
[[ "$(git rev-parse HEAD)" == "$(git rev-parse origin/main)" ]] || {
  echo "main is not in sync with origin/main" >&2; exit 1; }
git rev-parse -q --verify "refs/tags/v$version" >/dev/null && { echo "tag v$version exists" >&2; exit 1; }

notes="$(awk '/^## \[Unreleased\]/{f=1; next} /^## \[/{f=0} f' CHANGELOG.md | sed '/^[[:space:]]*$/d')"
[[ -n "$notes" ]] || { echo "CHANGELOG.md: the Unreleased section is empty" >&2; exit 1; }

today="$(date -u +%Y-%m-%d)"
sed -i -E "0,/^  VERSION [0-9]+\.[0-9]+\.[0-9]+/s//  VERSION $version/" CMakeLists.txt
sed -i -E "s|<releases>|<releases>\n    <release version=\"$version\" date=\"$today\"/>|" \
  packaging/linux/io.github.dev_taosif_net.Chobi.metainfo.xml
sed -i "s/^## \[Unreleased\]$/## [Unreleased]\n\n## [$version] - $today/" CHANGELOG.md

cmake -S . -B build-release-check -G Ninja -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build build-release-check >/dev/null
ctest --test-dir build-release-check -L unit --output-on-failure >/dev/null
[[ "$(build-release-check/src/chobi --version)" == "Chobi $version" ]] || {
  echo "version bump failed" >&2; exit 1; }

git add CMakeLists.txt CHANGELOG.md packaging/linux/io.github.dev_taosif_net.Chobi.metainfo.xml
git commit -q -m "Release $version"
git tag -a "v$version" -m "Chobi $version"
git push -q origin main "v$version"
echo "Pushed v$version. Follow the build: gh run watch \$(gh run list -w Release -L 1 --json databaseId -q '.[0].databaseId')"
