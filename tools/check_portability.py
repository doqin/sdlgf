#!/usr/bin/env python3
"""Portability lint for case-sensitive filesystems (Linux, macOS CS volumes).

Windows hides two classes of bug that break the moment the project is built or
run elsewhere:

  1. #include "..." that only resolves because the filesystem ignores case, or
     that uses backslashes.
  2. Asset paths in string literals ("assets/Foo.PNG") that only resolve because
     the filesystem ignores case, or that use backslashes.

Usage: python tools/check_portability.py
Exit code 1 if any problem is found (so it can be wired into CI).
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
SRC_DIRS = ['core', 'demo', 'example']
SKIP_DIRS = {'backends', 'misc', 'assets', 'Resources', 'build'}
SKIP_FILE_PREFIXES = ('imgui', 'imstb')

# Include search path used by the CMake targets (see core/ and demo/ CMakeLists).
INCLUDE_DIRS = [
    'core', 'core/Extras', 'includes', 'demo',
    'demo/cards/core', 'demo/cards/targeting', 'demo/cards/registry', 'demo/cards/defs',
    'demo/backends', 'demo/misc/cpp', 'cmake/shims',
]

_listing_cache = {}


def listing(path):
    if path not in _listing_cache:
        try:
            _listing_cache[path] = os.listdir(path)
        except OSError:
            _listing_cache[path] = []
    return _listing_cache[path]


def _walk(base, rel, fold):
    cur = base
    for part in rel.replace('\\', '/').split('/'):
        if part in ('', '.'):
            continue
        if part == '..':
            cur = os.path.dirname(cur)
            continue
        if fold:
            match = next((n for n in listing(cur) if n.lower() == part.lower()), None)
        else:
            match = part if part in listing(cur) else None
        if match is None:
            return False
        cur = os.path.join(cur, match)
    return os.path.exists(cur)


def exact_exists(base, rel):
    return _walk(base, rel, fold=False)


def ci_exists(base, rel):
    return _walk(base, rel, fold=True)


def source_files():
    for d in SRC_DIRS:
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, d)):
            dirnames[:] = [x for x in dirnames if x not in SKIP_DIRS]
            for f in filenames:
                if f.startswith(SKIP_FILE_PREFIXES):
                    continue
                if f.endswith(('.cpp', '.h', '.hpp')):
                    yield os.path.join(dirpath, f)


INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"([^"]+)"', re.M)
ASSET_RE = re.compile(r'"((?:[^"\\\n]|\\.)*?assets(?:\\\\|/)(?:[^"\\\n]|\\.)*?)"')


def read(path):
    with open(path, encoding='utf-8-sig', errors='replace') as fh:
        return fh.read()


def main():
    problems = []
    for path in source_files():
        rel_path = os.path.relpath(path, ROOT).replace('\\', '/')
        text = read(path)
        here = os.path.dirname(path)

        for m in INCLUDE_RE.finditer(text):
            inc = m.group(1)
            line = text.count('\n', 0, m.start()) + 1
            if '\\' in inc:
                problems.append(f'{rel_path}:{line}: include uses backslash: "{inc}"')
            bases = [here] + [os.path.join(ROOT, d) for d in INCLUDE_DIRS]
            if any(exact_exists(b, inc) for b in bases):
                continue
            if any(ci_exists(b, inc) for b in bases):
                problems.append(f'{rel_path}:{line}: include differs in case from file on disk: "{inc}"')
            # else: not one of our files (SDK header spelled with quotes): ignore

        for m in ASSET_RE.finditer(text):
            lit = m.group(1)
            line = text.count('\n', 0, m.start()) + 1
            path_str = lit.replace('\\\\', '\\')
            if '%' in path_str or '{' in path_str:
                continue  # formatted, can't check statically
            if '\\' in path_str:
                problems.append(f'{rel_path}:{line}: asset path uses backslash: "{path_str}"')
            norm = path_str.replace('\\', '/')
            norm = norm[norm.find('assets/'):]
            if norm.endswith('/'):
                continue
            demo = os.path.join(ROOT, 'demo')
            if exact_exists(demo, norm):
                continue
            if ci_exists(demo, norm):
                problems.append(f'{rel_path}:{line}: asset path differs in case from file on disk: "{norm}"')
            else:
                problems.append(f'{rel_path}:{line}: asset not found: "{norm}"')

    problems.extend(check_system_header_shadowing())

    unique = sorted(set(problems))
    for p in unique:
        print(p)
    print(f'\n{len(unique)} problem(s) found.')
    return 1 if unique else 0


# Headers that ship with compilers and SDKs. A project header whose name matches one of these
# ignoring case (core/Math.h vs <math.h>) shadows it on case-insensitive filesystems
# (Windows, default macOS) as soon as its directory is on the include path - which breaks
# the standard library itself, not just our code.
SYSTEM_HEADERS = {
    'assert.h', 'complex.h', 'ctype.h', 'errno.h', 'fenv.h', 'float.h', 'inttypes.h', 'iso646.h',
    'limits.h', 'locale.h', 'math.h', 'setjmp.h', 'signal.h', 'stdalign.h', 'stdarg.h', 'stdatomic.h',
    'stdbool.h', 'stddef.h', 'stdint.h', 'stdio.h', 'stdlib.h', 'stdnoreturn.h', 'string.h', 'tgmath.h',
    'threads.h', 'time.h', 'uchar.h', 'wchar.h', 'wctype.h', 'malloc.h', 'memory.h', 'process.h', 'io.h',
    'direct.h', 'conio.h', 'search.h', 'share.h', 'tchar.h', 'excpt.h', 'unistd.h', 'fcntl.h', 'dirent.h',
    'windows.h', 'winsock.h', 'winsock2.h', 'mmsystem.h', 'shellapi.h', 'commctrl.h', 'd3d9.h', 'dinput.h',
    'xaudio2.h', 'objbase.h', 'basetsd.h', 'wtypes.h', 'rpc.h', 'guiddef.h', 'cderr.h', 'dde.h',
    'direct.h', 'dos.h', 'ocidl.h', 'oleauto.h', 'pthread.h', 'regex.h', 'semaphore.h', 'syslog.h',
}


def check_system_header_shadowing():
    found = []
    for d in INCLUDE_DIRS:
        base = os.path.join(ROOT, d)
        for name in listing(base):
            if name.lower() in SYSTEM_HEADERS and os.path.isfile(os.path.join(base, name)):
                found.append(f'{d}/{name}: shadows the system header <{name.lower()}> on case-insensitive filesystems; rename it')
    return found


if __name__ == '__main__':
    sys.exit(main())
