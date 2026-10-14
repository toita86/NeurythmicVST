# Session ID
AE-2026-10-14-1

## Context
- Roadmap phase: Phase 8 — Cross-Platform Distribution
- Development task: a tag-triggered GitHub Actions release workflow building
  macOS (universal), Windows and Linux VST3 bundles, plus the versioning policy
  behind it: a first release `0.1.0` derived from the git tag, a state-schema
  version, a generated VST3 manifest, a main-menu version label, and expanded
  install documentation.
- Duration:
    - time: 2 hours
    - usage: 774 k tokens
    - cost: 0.07$
- Agent and model: opencode / deepseek-v4.1-flash
- Repository branch: ADD-phase_8

## Evidence
- Commit: none yet (changes uncommitted at time of writing)
- Changed files:
  - modified: CMakeLists.txt (`NEURYTHMIC_VERSION` cache variable feeding
    `juce_add_plugin(VERSION ...)`, `VST3_AUTO_MANIFEST TRUE`, module
    `NEURYTHMIC_VERSION` compile definition)
  - new: .github/workflows/release.yml (tag/dispatch matrix build, per-OS
    artefact naming, `softprops/action-gh-release`)
  - modified: neurythmic_plugin/include/Neurythmic/NetworkState.h,
    source/NetworkState.cpp (`Props::stateVersion`, `kStateVersion`,
    set in `initEmptyNetwork()`)
  - modified: neurythmic_plugin/source/NetworkController.cpp (`rebuild()`
    defaults a missing `stateVersion` to 1 for legacy presets)
  - modified: neurythmic_plugin/include/Neurythmic/MainMenu.h,
    source/MainMenu.cpp (small version label on the CPG tab)
  - new: test/source/VersionTest.cpp; modified: test/CMakeLists.txt
    (register test, add `NEURYTHMIC_VERSION` define)
  - modified: assets/README.txt, README.md (version/install/unsigned notes)
  - modified: docs/roadmap.md (§8.1–§8.4 rewritten)
- Tests: 176 passing (was 172; +4 — `VersionTest`)
- Prompt transcript: this conversation
- Screenshot or build log: dev build 176/176, clang-format clean; release
  preset built with `-DNEURYTHMIC_VERSION=0.1.0` → generated
  `moduleinfo.json` reports `"Version": "0.1.0"` and the POST_BUILD
  `Neurythmic.zip` contains the bundle + `README.txt`

## Factual account
The phase opened with a discussion of what "plugin version" actually means
here. The agent separated four independent notions: the component version baked
into the bundle by `juce_add_plugin` (what hosts cache and display), the release
tag, the preset/state schema, and the config/engine. The researcher chose
`0.1.0` as the first release (explicitly pre-1.0, no preset or automation
compatibility guarantee), decided the git tag should be the single source of
truth for the bundle version, confirmed the existing identity codes
(`Toit` / `Ntpl` / `Neurythmic`) are final, chose to add an explicit state-schema
version now, and asked for the VST3 manifest to be generated (`VST3_AUTO_MANIFEST
TRUE`). The researcher also asked for the version to appear as a small main-menu
label (CPG tab only) and for releases to be named per OS *and* architecture.

`docs/roadmap.md` §8.1–§8.4 were rewritten to record all of this before any code
was touched. The implementation then followed test-first: `VersionTest.cpp`
asserts the `NEURYTHMIC_VERSION` macro is defined and parses as
`major.minor.patch`, that `initEmptyNetwork()` carries the current
`stateVersion`, that a legacy tree lacking the property loads as version 1, and
that a state round-trip preserves it. `CMakeLists.txt` gained a
`NEURYTHMIC_VERSION` cache variable (default `0.0.0`) which the release workflow
overrides from the pushed tag (`${GITHUB_REF_NAME#v}`), a module compile
definition so the UI and tests can read it, and `VST3_AUTO_MANIFEST TRUE`.
`NetworkState` gained the property and constant; `NetworkController::rebuild()`
now stamps a default on trees that predate it. `MainMenu` shows `v0.1.0` at the
bottom of the CPG tab. A new `release.yml` builds the three platforms on tag
push (and on manual dispatch, which builds but does not publish), renaming the
POST_BUILD zip to `Neurythmic-v<ver>-{macOS-universal,Windows-x86_64,
Linux-x86_64}.zip` before uploading, and attaches them to a GitHub Release.

Two corrections during implementation: the test target compiles the plugin
sources directly rather than linking the module alias, so the `NEURYTHMIC_VERSION`
definition had to be added to `test/CMakeLists.txt` as well as the module
interface; and the agent initially considered flagging the macOS deployment
target as too old, but a check of JUCE 8.0.12's README confirmed `10.11` is still
supported, so the concern was dropped rather than acted on. The release preset
was configured and built once locally to prove the version flag flows end-to-end
into the generated manifest and the packaged zip.

## Immediate researcher reflection
- What did I expect? That Phase 8 would be mostly YAML plumbing, and that "the
  version" would be a single obvious number.
- What surprised me? How many distinct versions coexist in a plugin, and how
  much host behaviour (AU rescanning, plugin caching, automation recall) hangs
  off the bundle version and the identity codes rather than off anything in the
  audio code. The `0.0.0` scaffold value was not neutral — it changes how AU
  hosts treat the component.
- What did I accept, modify, or reject? Accepted the tag-derived version, the
  frozen identity codes, the explicit schema version and the generated manifest;
  modified nothing substantive from the plan; rejected notarization/code signing
  as out of scope.
- Where did my own JUCE, C++, DSP, or DAW knowledge matter? Knowing that hosts
  key automation and project recall on the plugin identity codes (so they must
  be frozen before the first release), that a non-zero AU version changes host
  caching, and how JUCE splits the version into `juce_add_plugin` metadata,
  `moduleinfo.json`, and the `JucePlugin_*` macros.
- How confident am I in the result, from 1 to 5, and why? 3.5 — the version
  wiring is verified locally through the real release preset and the manifest,
  and the state-schema change is unit-tested; but the GitHub Actions workflow
  itself has never executed, so the Windows/macOS runners, the MSVC environment
  setup, and the release publication remain unproven until the first tag is
  pushed.

## Agent-generated prompts and researcher answers
- Agent: "What should the first release version be?" Researcher: `0.1.0`.
- Agent: "Should the git tag or CMake own the version?" Researcher: derive it
  from the tag in CI (approach B), with a local fallback.
- Agent: "Are the identity codes final?" Researcher: keep `Toit` / `Ntpl` /
  `Neurythmic`.
- Agent: "Add an explicit state/preset schema version now or defer?" Researcher:
  add it now.
- Agent: "Generate the VST3 manifest or leave it off?" Researcher: set the flag
  to `TRUE`.
- Agent: "Per-OS artefact names, and should they include the architecture?"
  Researcher: per-OS, including the architecture.
- Agent: "Where should the version label live?" Researcher: CPG tab only.

## Provisional codes
- AI-assisted reverse engineering
- architectural translation
- verification burden
- trust calibration
- real-time safety
- preservation versus modernization

## AI involvement declaration
The agent organized the evidence and proposed questions and codes.
The factual and reflective account was reviewed by the researcher.
