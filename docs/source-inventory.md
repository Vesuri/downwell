# Windows source-build inventory

The local source of truth is `data/`, an untouched Steam installation of
Downwell app 360740. The directory is ignored by Git.

## Fingerprints

The files the Amiga slice's converter reads ([amiga-input-inventory.md](amiga-input-inventory.md)).

| File | SHA-256 |
|---|---|
| `data/data.win` | `c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8` |
| `data/Downwell.exe` | `340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085` |
| `data/language.ini` | `b56ff8dc2f9ff012fcab1af8757d2e765a3a2c1f972c1ba41ae405e8bd4e4c68` |
| `data/sfxfalling.ogg` | `7b82aa294300209ba772acd3a21af1bb6695d0421491b22c400a1d5e4f2e4209` |
| `data/amb1.ogg` | `bb1a6440193ecaf4160f5b1e5a773f054c1289554dd6adf43cfbbe42d0b37b7f` |
| `data/sfxbossfirsthit.ogg` | `07a3a0444fcfb2d57fc369ea93a360802e5e6a9afa923018aec048455772494e` |
| `data/audiogroup1.dat` | `c0d53595a4fcf7b8ca5d08d8deb4db4cb9a893c96b17954518c938a200cfce81` |
| `data/audiogroup2.dat` | `168eab2e92bf47f9b1354b2d44b22094c69ed039c891f172a63215a48d590d88` |
| `data/audiogroup3.dat` | `042188e65c982d15a6183e1fc3a7f70cfe811ed63fe30601e5cea6af90ccd6ea` |
| `data/audiogroup4.dat` | `dc972711b0e4e5747d196eff5692fc5502f9ec735fb0a9bae98002d1dc205083` |
| `data/audiogroup5.dat` | `3df0bc4ea1f8625be1363b6a71d4fb6f802da179f36d2d96a23c72dbb922aac2` |
| `data/audiogroup6.dat` | `6fe5a5ae2c328f348d1a3870cdb3a7002d89d98606b44d2e7f4764e0123a5893` |

## GameMaker identification

- VM bytecode, not YYC: `CODE`, `VARI`, and `FUNC` are populated.
- Bytecode/data format 17.
- Stored version is the non-specific `2.0.0.0`.
- Chunk features place it at GameMaker 2.3.6 or newer; timestamps and format
  indicate a GameMaker 2022.5-era build.
- Internal name: `Downwell_v1_0_5`.
- Logical window: 320x568.
- Data compilation timestamp: 2022-05-31 11:31:03 UTC.

## Resource counts

| Resource | Count |
|---|---:|
| VM code entries | 2,810 |
| variables | 4,638 |
| functions | 588 |
| script resource records | 618 |
| objects | 386 |
| rooms | 25 |
| sprites | 741 |
| texture-page items | 3,161 |
| embedded texture atlases | 2 |
| sounds | 361 |
| fonts | 8 |
| shaders | 62 |
| strings | 8,420 |

Sound payload accounting is exact: 348 entries in `data.win`, ten across six
`audiogroup*.dat` files, and three loose Ogg files.

The earlier low-level inspection reported 4,115 variables by interpreting the
first `VARI` word as a list count. `VARI` is not a normal count-prefixed chunk;
UndertaleModLib's parsed count of 4,638 is authoritative.

Future extraction manifests must repeat the input hashes and record the exact
extractor version/commit.
