#!/usr/bin/env python3
"""Converts images to 1-bit PNG files"""

from PIL import Image
import pathlib as pth

def convert(input: pth.Path | str, output: pth.Path | str | None=None):
    input = pth.Path(input)
    assert input.is_file()

    output = pth.Path(output) if output is not None else input.with_suffix('.png')
    assert input != output, 'Input and output files must differ'

    im = Image.open(input)
    conv = im.convert('1')
    conv.info = {}

    conv.save(output, optimize=True)
    return output, conv

if __name__ == '__main__':
    import argparse

    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.ArgumentDefaultsHelpFormatter
    )

    parser.add_argument('-o', '--outfile', default=None,
        help='Output file (deduced automatically if absent)')
    
    parser.add_argument('infile',
        help='Input file')

    opt = parser.parse_args()

    outf, res = convert(input=opt.infile, output=opt.outfile)
    print(f'Written {res.size[0]}x{res.size[1]} ({outf.stat().st_size} B): {outf}')

