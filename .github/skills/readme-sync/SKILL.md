---
name: readme-sync
description: "Keep README.md in sync with the current project state. Use when project files or structure have changed, after adding new features or build targets, when README.md might be outdated, or to verify docs reflect the actual codebase. Checks git status and recent commits, then updates README.md sections (build instructions, project structure, usage, requirements) as needed."
argument-hint: "Optional: describe what changed or which README section to review"
---

# README Sync

## When to Use

- New source files, directories, or build targets have been added
- Git status shows untracked or modified files that affect project structure
- Recent commits introduced features not yet documented
- README.md sections (build, usage, structure) appear stale or missing

## Procedure

### Step 1 — Inspect Git State

```bash
git status --short
git log --oneline -10
```

- Note untracked files and directories → these may need to be documented
- Scan commit messages for feature keywords (feat, add, implement) → those likely need README coverage

### Step 2 — Read Current README.md

Read the full README.md to understand what is already documented.

Identify missing or outdated sections by comparing against the git state from Step 1.

### Step 3 — Inspect Key Files

For any untracked or recently modified files relevant to usage or build, read them:

- `CMakeLists.txt` — extract project name, C++ standard, executables/libraries defined
- `src/main.cpp` or entry-point files — extract CLI flags, tool description
- Any config files (`.clang-format`, `vcpkg.json`, etc.) — note requirements

### Step 4 — Assess What Needs Updating

Use this checklist:

| Section                    | Needs update if…                                                      |
| -------------------------- | --------------------------------------------------------------------- |
| **Overview / Description** | Tool purpose or scope has changed                                     |
| **Requirements**           | New compiler version, CMake version, or dependencies introduced       |
| **Build**                  | CMakeLists.txt changed or build directory exists but isn't documented |
| **Usage**                  | New CLI flags, modes, or inputs added                                 |
| **Project Structure**      | New top-level directories added (`src/`, `include/`, `docs/`, etc.)   |

Sections that already accurately reflect the project **do not** need to be touched.

### Step 5 — Update README.md

Apply only the changes identified in Step 4. Keep existing accurate content intact.

- Use standard Markdown headings (`##` for sections)
- For build instructions, prefer a fenced `bash` code block
- For project structure, use a fenced tree block
- Do not add speculative or aspirational content — document only what exists

### Step 6 — Confirm

After editing, briefly state which sections were added or updated and why, so the user can review the changes.
