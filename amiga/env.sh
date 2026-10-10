#!/usr/bin/env bash
# Source this for the shared Amiga environment that AmigaXDev installs (~/.local/share/amiga):
# the toolchain and FS-UAE on PATH, the Kickstart and the FS-UAE helpers.  Every variable can be
# overridden from the environment.
#   . amiga/env.sh
# or from the amiga/ directory:
#   . env.sh
. "${AMIGA_ENV:-$HOME/.local/share/amiga/env.sh}"
