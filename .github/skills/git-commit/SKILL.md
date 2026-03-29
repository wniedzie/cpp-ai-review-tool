---
name: git-commit
description: 'Smart git commit workflow. Use when committing changes, creating branches, staging files, writing commit messages, or pushing to remote. Handles branch creation (feature/*, bugfix/*, docs/*), logical change grouping into separate commits, and conventional commit messages with short header and detailed description.'
argument-hint: 'Optional: describe what you are committing or specify branch type (feature/bugfix/docs)'
---

# Git Commit Workflow

## When to Use
- Ready to commit local changes
- Need to create a branch before committing
- Multiple unrelated changes need to be split into separate commits
- Want well-structured commit messages pushed to remote

## Procedure

### Step 1 — Check Branch Status

```bash
git branch --show-current
git status --short
```

- If on `main`, `master`, `develop`, or in a detached HEAD state → go to **Step 2**
- If already on a `feature/*`, `bugfix/*`, or `docs/*` branch → skip to **Step 3**

---

### Step 2 — Create a Branch

Determine the branch type from the nature of the changes:

| Change type | Branch prefix |
|---|---|
| New functionality | `feature/` |
| Bug fix | `bugfix/` |
| Documentation only | `docs/` |

If the branch name is not obvious from context, ask the user for a short descriptor. Then:

```bash
git checkout -b <prefix>/<short-kebab-name>
```

---

### Step 3 — Analyze All Changes

```bash
git status
git diff --stat HEAD
```

Inspect every file in the output. Group changed files into **logical bundles** where each bundle:
- Addresses a single feature, fix, or documentation topic
- Contains all files that belong together (e.g., source file + its test + its docs entry)

If there are files from **two or more unrelated topics**, split them into separate bundles — each bundle becomes its own commit.

> **Decision rule:** Would reverting just *this bundle* make sense as a standalone rollback? If yes, it is its own commit.

---

### Step 4 — Stage and Commit Each Bundle

Repeat the following for each logical bundle, in a sensible order (dependencies first):

**4a. Stage only the files in this bundle:**
```bash
git add <file1> <file2> ...
```

Use `git add -p <file>` for partial staging if one file contains changes from two topics.

**4b. Compose the commit message** using the format below.

**4c. Commit:**
```bash
git commit -m "<header>" -m "<detailed body>"
```

#### Commit Message Format

```
<type>(<scope>): <short imperative description>     ← header, ≤ 72 chars

<Paragraph explaining WHY the change was needed —
context, motivation, or problem being solved.>

<Paragraph explaining WHAT changed at a high level —
key decisions, approach, or trade-offs made.>

<BREAKING CHANGE: describe if applicable>
```

**Type** — choose based on the nature of the change:

| Type | When to use |
|---|---|
| `feat` | New feature or capability |
| `fix` | Bug fix |
| `docs` | Documentation only |
| `refactor` | Code restructure without behaviour change |
| `test` | Adding or fixing tests |
| `chore` | Build system, tooling, CI, dependency updates |

**Scope** — optional, identifies the module or area affected (e.g., `auth`, `parser`, `api`).

**Header** — imperative mood: "add", "fix", "update" — not "added" or "fixes".

---

### Step 5 — Push

After all commits are created:

```bash
git push -u origin <branch-name>
```

If the remote branch already exists:

```bash
git push
```

---

## Quality Checklist

Before each commit, verify:
- [ ] No debug/temporary code or leftover `TODO` in staged files
- [ ] Each commit is self-contained (the codebase is valid with just that commit applied)
- [ ] Header is ≤ 72 characters and uses imperative mood
- [ ] Body explains *why* the change was needed, not just *what* changed
- [ ] Branch prefix matches the type of change

---

## Examples

### Single logical change
```
feat(auth): add JWT refresh token rotation

The previous implementation reused tokens indefinitely, creating a
security window if a token was intercepted before expiry.

Added a rotation mechanism that issues a new refresh token on every
successful use and immediately invalidates the previous one in Redis.
```

### Two unrelated changes — split into two commits

**Commit 1** (staged: `parser.cpp`, `parser_test.cpp`):
```
fix(parser): handle empty input without crashing

Empty strings triggered an unchecked dereference at the start of the
tokenization loop, causing a segfault on valid user input.

Added an early-return guard and a corresponding regression test.
```

**Commit 2** (staged: `README.md`, `docs/API.md`):
```
docs(api): document parser edge cases and error behaviour

Downstream consumers had no documented guidance on what inputs the
parser rejects and how errors are surfaced.

Added an "Edge Cases" section to README and extended the API reference
with error code descriptions and example invalid inputs.
```
