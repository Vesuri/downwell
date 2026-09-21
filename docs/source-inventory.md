# Windows source-build inventory

The local source of truth is `data/`, an untouched Steam installation of
Downwell app 360740. The directory is ignored by Git.

## Fingerprints

| File | SHA-256 |
|---|---|
| `data/data.win` | `c07daf5fbe940ae0a19503b5ba5137a56ee492753b1cea34b276a5cf7545fdb8` |
| `data/Downwell.exe` | `340b43dae1a18d2861cba9f6a9cdbb92f3754699854e0dc1a1ca367a8cd11085` |

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
