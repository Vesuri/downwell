#!/usr/bin/env python3
"""Compile-and-link wrapper with per-source objects, for the test programs.

    python3 tools/cxx-objects.py COMPILER ARG... -o OUTPUT SOURCE...

Takes the same command line as a one-step `clang++ ... -o prog a.cpp b.cpp`.
Each source is compiled once into build/obj/<flags>/<source>.o, where
<flags> is a digest of the compiler and every argument except the output,
the sources and link-only arguments. An object is rebuilt only when it is
missing, when its own -MMD dependency file is missing, or when a file that
dependency list names is newer than it. Then the objects are linked with
the original arguments. Thirty tests that compile GameplayWorld.cpp with the
same flags therefore share one object, and a header change recompiles only
the objects that include that header.

Parallel make: an object is built under an exclusive lock on <object>.lock
and renamed into place, so two links that need it compile it once. Commands
that already compile (-c, -E, -S, -fsyntax-only, -M) or have no source run
unchanged.
"""
import fcntl
import hashlib
import os
from pathlib import Path
import subprocess
import sys

SOURCE_SUFFIXES = ('.cpp', '.cc', '.cxx', '.c', '.m', '.mm')
PASS_THROUGH = {'-c', '-E', '-S', '-fsyntax-only', '-M', '-MM'}
ROOT = Path('build/obj')


def link_only(argument):
    return argument.startswith(('-l', '-L', '-Wl,')) or argument in ('-framework', '-pthread')


def split(arguments):
    """Return (compile flags, link-only flags, sources, other inputs, output)."""
    compile_flags, link_flags, sources, inputs, output = [], [], [], [], None
    i = 0
    while i < len(arguments):
        a = arguments[i]
        if a == '-o':
            output = arguments[i + 1]; i += 2; continue
        if a == '-framework':
            link_flags += arguments[i:i + 2]; i += 2; continue
        if a in ('-MF', '-MT', '-MQ'):
            i += 2; continue
        if a in ('-MMD', '-MD', '-MP'):
            i += 1; continue
        if a.endswith(SOURCE_SUFFIXES) and not a.startswith('-'):
            sources.append(a)
        elif a.endswith(('.o', '.a', '.dylib')) and not a.startswith('-'):
            inputs.append(a)
        elif link_only(a):
            link_flags.append(a)
        else:
            compile_flags.append(a)
        i += 1
    return compile_flags, link_flags, sources, inputs, output


def dependencies(path):
    text = path.read_text().replace('\\\n', ' ')
    first = text.split('\n', 1)[0]
    if ':' not in first:
        return None
    return [d for d in first.split(':', 1)[1].split() if d]


def stale(obj, dep):
    if not obj.exists() or not dep.exists():
        return True
    built = obj.stat().st_mtime
    names = dependencies(dep)
    if names is None:
        return True
    for name in names:
        try:
            if os.stat(name).st_mtime > built:
                return True
        except FileNotFoundError:
            return True
    return False


def build(compiler, flags, source, directory):
    obj = directory / (source + '.o')
    dep = directory / (source + '.d')
    obj.parent.mkdir(parents=True, exist_ok=True)
    if not stale(obj, dep):
        return obj
    with open(str(obj) + '.lock', 'w') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if not stale(obj, dep):
            return obj
        tmp_obj = obj.with_name(obj.name + f'.{os.getpid()}.tmp')
        tmp_dep = dep.with_name(dep.name + f'.{os.getpid()}.tmp')
        # -MT names the real object so the list reads the same after rename.
        result = subprocess.run([compiler, *flags, '-MMD', '-MF', str(tmp_dep), '-MT', str(obj),
                                 '-c', '-o', str(tmp_obj), source])
        if result.returncode:
            for t in (tmp_obj, tmp_dep):
                t.unlink(missing_ok=True)
            sys.exit(result.returncode)
        os.replace(tmp_dep, dep)
        os.replace(tmp_obj, obj)
    return obj


def main():
    compiler, *arguments = sys.argv[1:]
    compile_flags, link_flags, sources, inputs, output = split(arguments)
    if PASS_THROUGH & set(arguments) or not sources or output is None:
        os.execvp(compiler, [compiler, *arguments])
    key = hashlib.sha256('\0'.join([compiler, *compile_flags]).encode()).hexdigest()[:16]
    directory = ROOT / key
    directory.mkdir(parents=True, exist_ok=True)
    flags_file = directory / 'flags'
    if not flags_file.exists():
        flags_file.write_text(' '.join([compiler, *compile_flags]) + '\n')
    objects = [str(build(compiler, compile_flags, source, directory)) for source in sources]
    # -x none: a `-x c++` among the flags must not make the objects sources.
    result = subprocess.run([compiler, *compile_flags, '-x', 'none', *objects, *inputs, *link_flags,
                             '-o', output])
    sys.exit(result.returncode)


if __name__ == '__main__':
    main()
