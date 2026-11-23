# Release icon assets

Base64 keeps the binary icons as text; packaging decodes them into standard
.info files. No image data is changed. Copied from Wing Commander 2's
`release/icons` (identical to Prince of Persia 2's), which has them from
Secret Weapons of the Luftwaffe, Pokeri and Slicks.

- `drawer.info.b64`: WHDLoad Install Template / `Xxx Install.info`, packaged as
  `Downwell Install.info`.
- `install.info.b64`: Rescue on Fractalus / whdload / `RoF Install/Install.info`.
- `game.inf.b64`: the same release's `RoF.inf`, packaged as `Downwell.inf`.
- `readme.info.b64`: the same release's `ReadMe.info` (MultiView default tool).

The released reference archive is https://whdload.de/games/RescueOnFractalus.lha.

Packaging (`tools/package_release.py`) changes only the Install icon's
length-prefixed APPNAME field (to Downwell; default tool Installer,
LOG=FALSE, PRETEND=FALSE, MINUSER=AVERAGE, no SCRIPT) and the game icon's
`do_Type`, from a project icon (4) to a tool icon (3): Downwell is a native
program, not a WHDLoad slave, so its icon has no default tool and no tool
types. Installer copies it to `Downwell.info` (only when the drawer has none,
so a placed icon keeps its position) and sets its stack to 8192 bytes.
