# Release

The Amiga release ships no game data. The player installs it from their own
Windows Steam copy of Downwell (1.0.5) with `DownwellInstallData`
(`tools/install-data`), which the Installer script runs. The pipeline follows
X-Wing's (`docs/release.md` there) and Wing Commander 2's, on AmigaXDev's
`amigaxdev.release`.

- **Version:**
  - `VERSION` holds `0.1`.
  - The literal version and date 26.11.2026 appear in `release/Install`'s
    `$VER`, in the converter's `$VER` (`tools/install-data/install_config.h`)
    and in the ReadMe's History. `tools/package_release.py` stamps the
    archive with that DOS date, and `tools/check_release.py` asserts them.
- **Program:** the stripped shipping build (`make -C amiga clean`, then
  `STRIP=1`): no verify, probe, profile or reference code.
- **Package:** `make package-release` builds the program and remakes the
  converter, then `tools/package_release.py` writes
  `build/release/Downwell-<VERSION>.lha` from an explicit allowlist under
  `Downwell Install/` with its drawer icon: `Downwell`, its tool icon template
  `Downwell.inf`, `DownwellInstallData`, `Install`, `Install.info`, `ReadMe`,
  `ReadMe.info`, the converter's three licences, and `Downwell.fields` and
  `Downwell.columns` (our own computation). It refuses any member named like
  an installed data file and any member whose SHA-256 is one of the player's
  files or of a file derived from them.
- **Audit:** `tools/check_release.py` decodes every member with Lhasa, checks
  the CRCs, the exact member set and identity with the build outputs, the
  HUNK and icon magics, the version strings and the ReadMe's sections, and
  scans every member for any 64-byte block, forward or byte-reversed, of the
  player's files (`tools/check-original-bytes.py`).
- **Installer:** `make test-installer` runs `release/Install` in Installer 43
  on the emulated A1200 with 4 MB Fast, in every mode (fresh, keep, replace,
  incomplete, remove, wrong), and starts the installed game from its icon
  and from a Shell (`tools/test_release_installer.py`; `--archive` takes the
  members from a built archive).
- **Make:**
  - `make release` refuses a tree with uncommitted changes. It runs
    `package-release`, copies the archive to `dist/`, audits it there and
    writes its `.sha256`. It refuses to replace an archive in `dist/`.
  - `make publish` remakes the build outputs at HEAD, requires the archive in
    `dist/` to equal them, audits it and checks the `.sha256`.

## Publishing

`make publish` runs `tools/publish_release.py` (from X-Wing's).

- **Dry run:** without `PUBLISH=1` it prints the tag, title, files and notes.
- **Publishing:** with `PUBLISH=1` it also checks the GitHub account (Vesuri)
  and that HEAD is on `main` on GitHub. It pushes a lightweight tag, the
  version, on HEAD, and creates the GitHub release
  `Downwell <version> (dd.mm.yyyy)`, marked latest.
- **Notes:** the early-alpha notice, then the version's ReadMe history items.
- **Assets:** it uploads the archive and its `.sha256`, then downloads the
  archive again to compare.

## 0.1

The world 1 slice, levels 1-1 to 1-3 with sound effects and no music, partly
optimized for the A1200 with 4 MB Fast. Its purpose is to announce the
project. Gate: `make release` (package and audit) and `make test-installer`
(every mode, both starts) on this build.

Built by `make release` from `7c086dbc` on 26.11.2026. It replaced the first
0.1 archive, built from `2d285dff` (709,639 bytes, SHA-256 `c08cc30d…5a07cdb2`),
whose ReadMe still gave AGA and an older speed note; the program and the
converter are unchanged. `make test-installer` passed every mode and both
starts on the first archive:

| Artifact | SHA-256 |
|---|---|
| `Downwell-0.1.lha`, 709,645 bytes | `958d57e7eac6cc052ff90384cf19204b9e3bce2bb0d5883a822aed4b3dbf0a35` |
| `Downwell` (exe) | `f3743aa5ed5ba6d8e3b598e03273d1ec6e73ebdf2b5d7cef415328336284685a` |
| `DownwellInstallData` | `a5b48787f8b781a5cb59ad7f20f6b939579574974d01144bcd1221776d79d9dd` |
