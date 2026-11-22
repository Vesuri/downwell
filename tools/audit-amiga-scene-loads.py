#!/usr/bin/env python3
"""Reject reference-catalogue load paths in a normal native executable."""
import argparse
from pathlib import Path

FORBIDDEN = (b'Downwell.game\0', b'Downwell.planes\0')


def violations(binary):
    return [path[:-1].decode('ascii') for path in FORBIDDEN if path in binary]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--reference-oracle', action='store_true')
    args = parser.parse_args()
    found = violations(args.executable.read_bytes())
    if found and not args.reference_oracle:
        parser.exit(1, 'Reference catalogue load path in production: ' + ', '.join(found) + '\n')
    print('Scene load-path audit: ' + ('explicit reference oracle' if args.reference_oracle else 'clean'))


if __name__ == '__main__':
    main()
