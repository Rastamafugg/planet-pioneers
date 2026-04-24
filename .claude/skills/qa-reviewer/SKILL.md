---
name: qa-reviewer
description: Planet Pioneers role for verification, review, regression checking, acceptance, and completion readiness. Mandatory closing role after any implementation task.
---

# QA Reviewer role

Used when the task is primarily about verification, review, regression checking, acceptance, or completion readiness — and **mandatory as the closing role after any implementation task**.

## Responsibilities

- Confirm the change meets the stated objective and respects the scope boundaries set by `project-management`.
- Check for regressions in adjacent code and shared subsystems. Consult the wiki's platform pages ([wiki/platform/](../../../wiki/platform/)) and lessons learned ([wiki/implementation/lessons-learned.md](../../../wiki/implementation/lessons-learned.md)) for failure modes to probe.
- **Verify build-script and disk-image discipline** per [wiki/implementation/build-workflow.md](../../../wiki/implementation/build-workflow.md) — in particular that `src/script/buildc` stays complete, `src/script/patchc` is minimal and task-specific, and `disks/ppsrc.dsk` was not modified unless the task specifically required it.
- **Spot-check DCC compatibility** of any C changes against [wiki/platform/dcc.md](../../../wiki/platform/dcc.md).
- **Confirm wiki maintenance was done for the task** — new sources ingested, new lessons recorded, decisions captured with rationale, [wiki/index.md](../../../wiki/index.md) updated, [wiki/log.md](../../../wiki/log.md) appended.

## Output

A short verdict: what was verified, what regressions were checked, any residual risk, and any wiki/build-script follow-ups.
