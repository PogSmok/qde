<!--
  Title format: <type>(<scope>): <subject>
  |              |         |          |
  |              |         |          +-> Short description, present tense, not capitalized, no period
  |              |         +------------> Scope: parser, simulator, ui, renderer, etc.
  |              +----------------------> Type: see list below
  |
  Types:
    feat      New feature or capability. Triggers minor version bump.
    fix       Bug fix correcting unintended behavior. Triggers patch bump.
    refactor  Restructuring code without fixing a bug or adding a feature.
    test      Adding or correcting tests. No production code change.
    docs      Documentation only; README, comments, changelogs.
    chore     Maintenance; CI config, build scripts, dependency updates.
    perf      Performance improvement with no behavior change.
    style     Formatting, whitespace, semicolons — no logic change.
    build     Changes to build system or external dependencies.
    ci        CI configuration files and scripts only.
    revert    Reverts a previous commit. Reference hash in the body.

  Examples:
    feat(parser): add support for CNOT gate syntax
    fix(simulator): correct phase calculation for T gate
    chore(ci): add sanitizer build step
-->

## Why

<!-- Explain the motivation behind this PR. What problem does it solve?
     What would happen if this wasn't merged? Do NOT describe what changed
     — the diff does that. Wrap at 72 characters.
     For reference, the line below has exactly 72 characters.
########################################################################

-->


---

## Checklist

- [ ] My branch follows the gitflow naming convention (`feature/`, `release/`, `hotfix/`, `fix/`)
- [ ] I have performed a self-review of my code
- [ ] I have added tests that prove my fix or feature works
- [ ] All existing tests pass locally
- [ ] I have updated documentation if needed

---

## Breaking changes

<!-- If this PR introduces breaking changes, describe them here.
     Otherwise delete this section.

     BREAKING CHANGE: <what changed and what callers must update>
-->

---

## Related issues

<!-- Closes #<issue number> -->