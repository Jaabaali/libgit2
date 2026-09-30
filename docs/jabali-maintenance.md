# Jabali libgit2 maintenance

This public fork supports [Jabali NodeGit](https://github.com/Jaabaali/nodegit).
It is based on upstream **v1.9.7**, with the NodeGit changes carried forward from
our original 1.9.4 port. No upstream pull requests are currently planned.

## Branches and tags

- `main`: upstream development; do not merge NodeGit patches here.
- `jabali/1.9`: maintained stable 1.9 line with NodeGit patches.
- `fix/*` and `update/*`: review branches targeting `jabali/1.9`.
- Upstream tags such as `v1.9.7` retain their original commit IDs.
- Patched releases use distinct tags such as `v1.9.7-jabali.1`.

NodeGit must pin an exact commit from this fork, not an upstream tag. The
checkout-options extension changes the C structure layout, so consumers must
build against this fork's headers and library together.

## Patch inventory

| Change | Original NodeGit commit | Purpose |
| --- | --- | --- |
| Disabled checkout filters | `624e1e610b4b37b35128fc16cac484e27114d2f4` | Allows named filters to be skipped during checkout. |
| Index timestamp comparison | `3e5e9531ef014d99aa5197a92da7dd96320dbcfc` | Avoids repeated work when an index lacks nanosecond timestamps. |
| Unquoted patch paths | `29171a2a718f94382d30d804880c3ef695cd0ae1` | Parses diff headers containing spaces; includes allocation-failure handling. |
| Custom thread-local storage | `de29c1592139b762ab4950e5830d90e409e2da95` | Carries the caller's context into libgit2 worker threads. |
| Parallel checkout | `f48847944ace20a4b3eafe7fc1bcc3f331f0c6d6` | Writes regular files concurrently, retaining serial symlink handling. |

The fork also carries focused regression tests and CI configuration. Keep fixes
in the relevant patch area and avoid unrelated upstream refactoring.

## Updating upstream

Check upstream stable releases at least monthly and promptly after security
announcements. Use stable release tags, not upstream `main`, for this line.

```sh
git fetch upstream --tags
git switch jabali/1.9
git switch -c update/libgit2-VERSION
git merge --no-ff vVERSION
```

Resolve conflicts while preserving the patch inventory, update the base version
in this document and README, then open a PR **in Jaabaali/libgit2**, targeting
`jabali/1.9`. Merge after CI and the NodeGit integration checks pass. Preserve
merge ancestry rather than squashing upstream updates, so subsequent updates
can identify changes already incorporated. A new upstream minor release should
start a new `jabali/X.Y` line and receive the same patch and compatibility audit.

`upstream` should point to `https://github.com/libgit2/libgit2.git`; `origin`
should point to `git@github.com:Jaabaali/libgit2.git`. Fetching upstream tags does
not publish them to the fork; push only the tags needed for releases.

## Validation

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_WERROR=ON \
  -DREGEX_BACKEND=builtin -DDEPRECATE_HARD=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure -R '^(offline|util)$'
```

Repeat with a separate build directory and `-DUSE_THREADS=OFF`. CI runs Linux
with and without threads, macOS, and Windows. Network-dependent tests are not
part of this matrix; validate SSH/HTTPS clone, fetch, and push through NodeGit
before shipping. Also exercise checkout/filter callbacks, diff/patch parsing,
commit/index operations, and shutdown in the supported Electron runtimes.

Publish a patched tag only after these checks, then update NodeGit's submodule
URL and commit. C-library CI alone does not establish NodeGit/Electron support.
