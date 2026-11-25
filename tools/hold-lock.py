#!/usr/bin/env python3
"""Take an exclusive flock on an inherited file descriptor, then exit.

    exec 9>>tmp/amiga.lock && python3 tools/hold-lock.py 9 && step1 && step2

A flock belongs to the open file description, not to this process: the
calling shell still holds fd 9, so the lock stays held until that shell
(and every child that inherited fd 9) has exited. The Makefile's
AMIGA_SERIAL prefix uses it so that recipes which clean and rebuild amiga/
or run the emulator never overlap under make -j, from one make or several.

    python3 tools/hold-lock.py 9 --try

takes the lock only if it is free (exit status 1, without waiting, if not).
"""
import fcntl
import sys

fd = int(sys.argv[1])
if sys.argv[2:] == ['--try']:
    try:
        fcntl.flock(fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
    except BlockingIOError:
        sys.exit(1)
else:
    fcntl.flock(fd, fcntl.LOCK_EX)
