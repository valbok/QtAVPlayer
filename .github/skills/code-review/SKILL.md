---
name: code-review
description: Review a GitHub pull request for correctness, regressions, tests, and QtAVPlayer-specific integration issues using the GitHub CLI.
---

# Pull Request Code Review

Use this skill when asked to review a pull request in this repository. Perform the
review with `gh`; do not require the pull request branch to be checked out unless
runtime validation is necessary.

## Gather the pull request context

If a pull request number is not supplied, determine it from the current branch:

```bash
gh pr view --json number,title,body,baseRefName,headRefName,files
```

For a supplied pull request, replace `<pr>` in the following commands:

```bash
gh pr view <pr> --json number,title,body,baseRefName,headRefName,files,commits
gh pr diff <pr> --name-only
gh pr diff <pr> --patch
gh pr checks <pr>
```

Read the changed code and the directly related callers, declarations, tests, and
build definitions. Use `gh api` to inspect review discussion when it affects the
correctness of the proposed change:

```bash
gh api repos/{owner}/{repo}/pulls/<pr>/comments --paginate
gh api repos/{owner}/{repo}/pulls/<pr>/reviews --paginate
```

Obtain `{owner}` and `{repo}` from `gh repo view --json nameWithOwner`.

## Review focus

Report only actionable, high-confidence defects introduced by the pull request.
Prioritize behavior and regressions over style.

- Validate media lifecycle and state transitions: loading, playing, pausing,
  seeking, stream switching, end of media, and errors.
- Trace ownership and thread affinity across the demuxer and decoder threads.
  Pay special attention to direct frame-signal connections, FFmpeg object
  lifetimes, queued work, and shutdown races.
- Check FFmpeg calls for error handling, correct time bases, packet/frame
  ownership, and hardware-frame compatibility.
- Ensure public API changes preserve source and binary expectations where
  applicable, and update both public headers and implementation.
- When sources, headers, resources, feature flags, or dependencies change,
  verify that the QMake (`.pri`/`.pro`) and CMake build definitions remain
  equivalent.
- Check platform-specific hardware paths and relevant Qt 5/Qt 6 compatibility
  guards.
- Confirm changed behavior has a focused integration test, or identify the
  concrete missing test when its absence could permit a regression.

Do not report formatting preferences, speculative risks, pre-existing issues, or
missing tests that do not exercise a changed behavior.

## Findings

Each finding must include:

1. A priority: `P0` (release-blocking), `P1` (urgent), `P2` (normal), or `P3`
   (minor).
2. The exact changed file and line range.
3. A concise explanation of the failure mode, including the input, state, or
   platform required to trigger it.
4. A specific remediation direction.

Prefer inline comments for findings tied to changed lines. If a defect spans
multiple files or cannot be anchored to a changed line, put it in the review
summary and identify all affected locations.

If no qualifying findings exist, state that the pull request has no blocking
issues. Do not invent findings to make the review appear more thorough.

## Publish the review

Do not submit a review, approve, or request changes unless explicitly asked.
When asked to publish, use the requested review state:

```bash
gh pr review <pr> --comment --body "<summary>"
gh pr review <pr> --approve --body "<summary>"
gh pr review <pr> --request-changes --body "<summary>"
```

Use the GitHub API for inline comments only after verifying the path, changed
line, and diff side. Never post a comment that is not backed by the inspected
diff.
