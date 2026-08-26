# Regression Review & Fix Plan

Scope: commits `690aaa9` (Support Deskflow remote input sessions) and `a010c1a`
(--self-test-key) by snapple42@gmail.com. Full static review done 2026-08-25.
**No build/test verification has happened yet — item 0 blocks everything else.**

---

## 0. Establish a build + test loop (BLOCKER)

The machine has no Qt6 dev packages; `cmake` configure fails. No CI exists in the
repo. Until this is fixed, "no regressions" is a claim based on eyeballing only.

- [x] Get a Qt6 toolchain working: either install `qt6-base-dev`, or use a
      container / nix-shell with Qt6 + libei + libxkbcommon + wayland deps.
- [x] Configure and build: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build`
- [x] Run unit tests (`tests/security_policy_test.cpp`, systemd-unit metadata test).
- [x] Add a GitHub Actions workflow that does the above on every push so this
      never has to be asked again.

## 1. Fix stale xkb state after compositor keymap change (bug, small diff)

File: `src/wayland_input.cpp` / `.hpp`

`keyboardKeymap()` updates `m_seatKeymapText` but `ensureKeymapState()` builds
`m_xkbKeymap` / `m_xkbState` once and never rebuilds them. After a layout switch,
modifier/group state reported to EIS clients is decoded against the old keymap.

- [x] When `m_seatKeymapText` changes in `WaylandInput::keyboardKeymap()`,
      invalidate: unref and null out `m_xkbState` (and optionally `m_xkbKeymap`)
      so the next key event rebuilds from the new keymap.
- [x] Also reset cached modifier baseline if needed so `sendModifiers()` still
      detects the change.
- [ ] Verify manually against a live Hyprland session: switch layouts mid
      Deskflow/KDE Connect session, confirm remote modifiers stay correct.

## 2. Cache the fallback keymap (perf edge case)

File: `src/wayland_input.cpp`, `keymapText()`

If the seat never delivers a keymap, the function compiles a default xkb keymap
every call without caching it — every keystroke then costs a full
`wl_display_roundtrip` plus an xkb context/keymap allocation.

- [x] Cache the compiled fallback (e.g., store it in `m_seatKeymapText` or a
      separate member once generated).
- [x] Confirm no behavior change when the compositor *does* send a keymap.

## 3. README honesty pass on sandbox downgrade (docs)

Files: `README.md`

Removing `PrivateTmp` / `ProtectSystem=strict` / `ProtectHome` is a real loss of
filesystem confinement; the added sysctl-style hardening
(`SystemCallFilter=@system-service`, `RestrictNamespaces`, ...) does not replace it.

- [x] State plainly in the security section that FS sandboxing was traded away to
      keep `/proc/<pid>/exe` caller verification working under a user manager.
- [x] Document what remains and what was given up, rather than "hardened differently".

## 4. Deskflow install-path coverage (expected user reports)

File: `src/security_policy.hpp` (`isAllowedDeskflowExecutablePath`)

Flatpak installs (`/app/bin/deskflow`) and custom builds (`~/.../deskflow`)
fail caller verification silently — symptom is "no input, no error".

- [x] Decide: allow Flatpak paths (`/app/bin/deskflow` exact match; `/usr/lib/extensions/...` rejected as too broad) (`/app/bin/deskflow*`, `/usr/lib/extensions/...`)
      or reject deliberately.
- [x] Either way, emit a clear log line when fallback verification fails due to an
      unrecognized executable path, including the path observed.

## 5. Fragility note for future maintainers (one-line fix)

File: `src/eis_input_bridge.cpp` (`dispatch()` path → `addKeyboard()`)

`wl_display_roundtrip` is invoked inside the EIS socket-notifier callback. Safe
today only because there is no Wayland-side socket notifier; adding one later
creates re-entrant dispatch hazards.

- [x] Add a comment at the `keymapText()` call site in `addKeyboard()` warning
      that this performs nested Wayland dispatch and must not be called from a
      Wayland listener callback.

## 6. Manual regression checklist (live session, after items 1–2)

KDE Connect must remain unaffected — verify explicitly:

- [ ] KDE Connect remote input: keys, mouse motion, click, scroll all work.
- [ ] Modifier lock state (caps lock / num lock) reflected correctly on phone.
- [ ] Deskflow server: motion, buttons, scroll on absolute pointer device.
- [ ] Deskflow: keyboard input maps correctly; switch layouts mid-session
      (validates item 1).
- [ ] Empty app id from verified KDE Connect daemon still allowed; unknown
      executable rejected (security policy unchanged for KDE Connect paths).
- [ ] `--self-test-key <code>` presses/releases expected key.
- [ ] systemd unit starts clean under `systemd --user`; portal requests succeed
      with the new `SystemCallFilter` set.

## Known-accepted nits (no action required now)

- `--self-test-key` accepts out-of-range evdev codes; compositor ignores them.
- `m_keyStateChanged` is set even when `keyboardKeycode()` fails — harmless.
- Named Deskflow app ids bypass caller verification — consistent with existing
  KDE Connect exact-match ids; trust anchors on xdg-desktop-portal app-id
  resolution plus the trusted-caller check on the backend D-Bus interface.
