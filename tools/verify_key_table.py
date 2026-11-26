#!/usr/bin/env python3
"""Verify core/Keys.cpp against SDL's own Windows scancode table.

SDL's src/events/scancodes_windows.h is indexed exactly like DirectInput key codes
(low byte of the set-1 scan code, +0x80 for 0xE0-prefixed keys), so for every
{ DIK_x, SDL_SCANCODE_y } row, table[value_of(DIK_x)] must be SDL_SCANCODE_y.

Usage: python tools/verify_key_table.py <path to SDL's scancodes_windows.h>
"""
import os
import re
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
# Deliberate departures from SDL's table (see comments in Keys.cpp).
EXCEPTIONS = {'DIK_PAUSE': 'SDL_SCANCODE_PAUSE'}


def sdl_table(path):
    text = open(path, encoding='utf-8').read()
    entries = re.findall(r'/\*0x([0-9A-Fa-f]+)\*/\s*(SDL_SCANCODE_\w+)', text)
    table = {}
    for pos, (comment, name) in enumerate(entries):
        c = int(comment, 16)
        idx = (c & 0xFF) | (0x80 if c > 0xFF else 0)   # packing used by SDL_windowsevents.c
        if idx != pos:
            sys.exit(f'unexpected SDL table layout at position {pos} (comment 0x{c:x})')
        table[pos] = name
    if len(table) != 256:
        sys.exit(f'expected 256 entries in SDL table, found {len(table)}')
    return table


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    table = sdl_table(sys.argv[1])

    values = {}
    for line in open(os.path.join(ROOT, 'core', 'input', 'Keys.h'), encoding='utf-8'):
        m = re.match(r'\s*#\s*define\s+(DIK_\w+)\s+(0x[0-9A-Fa-f]+)\b', line)
        if m:
            values[m.group(1)] = int(m.group(2), 16)

    src = open(os.path.join(ROOT, 'core', 'input', 'Keys.cpp'), encoding='utf-8').read()
    pairs = re.findall(r'\{\s*(DIK_\w+)\s*,\s*(SDL_SCANCODE_\w+)\s*\}', src)
    problems = []
    seen_sdl = {}
    for dik, sdl in pairs:
        if dik not in values:
            problems.append(f'{dik}: not defined in Keys.h')
            continue
        expected = EXCEPTIONS.get(dik, table[values[dik]])
        if sdl != expected:
            problems.append(f'{dik} (0x{values[dik]:02X}): table says {expected}, Keys.cpp says {sdl}')
        if sdl in seen_sdl:
            problems.append(f'{sdl} is the target of both {seen_sdl[sdl]} and {dik}')
        seen_sdl[sdl] = dik

    for p in problems:
        print('FAIL', p)
    print(f'{len(pairs)} key mappings checked against SDL table, {len(problems)} problem(s).')
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
