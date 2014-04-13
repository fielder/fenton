#!/usr/bin/env python

import sys
import os
import struct
import itertools
import collections

import texutil
import wad

RGB = collections.namedtuple("RGB", "r g b")

OUT_VERSION = 1
OUT_NUMLEVELS = 4

PALNUM = 0


class Pic(object):
    width = 0
    height = 0
    pixels = None # list of RGB's
    mask = None # list of ints, 0 for transparent pixel, 1 for opaque

    def pixel(self, x, y):
        if x < 0 or x >= self.width or y < 0 or y >= self.height:
            raise ValueError("pixel off image (%d, %d)" % (x, y))
        return self.pixels[y * self.width + x]

    def maskval(self, x, y):
        if not self.mask:
            raise Exception("no mask")
        if x < 0 or x >= self.width or y < 0 or y >= self.height:
            raise ValueError("mask pixel off image (%d, %d)" % (x, y))
        return self.mask[y * self.width + x]

    def chunkColor(self, x, y, chunk_width, chunk_height):
        """
        Find the average color of a sub-rectangle in the image.
        """

        r = 0
        g = 0
        b = 0

        for cy in xrange(chunk_height):
            for cx in xrange(chunk_width):
                p = self.pixel(x + cx, y + cy)
                r += p.r
                g += p.g
                b += p.b

        r /= (chunk_width * chunk_height)
        g /= (chunk_width * chunk_height)
        b /= (chunk_width * chunk_height)

        return RGB(r, g, b)

    def chunkOpacity(self, x, y, chunk_width, chunk_height):
        """
        Find the average opacity of a sub-rectangle in the image.
        """

        tot = 0.0

        for cy in xrange(chunk_height):
            for cx in xrange(chunk_width):
                tot += self.maskval(x + cx, y + cy)

        return tot / (chunk_width * chunk_height)


_pixel_to_rgb = {}
def _loadPal():
    if not _pixel_to_rgb:
        for idx in xrange(256):
            pixelbyte = chr(idx)
            rgb_bytes = texutil.palettes[PALNUM][pixelbyte]
            r = ord(rgb_bytes[0])
            g = ord(rgb_bytes[1])
            b = ord(rgb_bytes[2])
            _pixel_to_rgb[pixelbyte] = RGB(r, g, b)

def rgbForPixel(pixel):
    _loadPal()
    return _pixel_to_rgb[pixel]

def rgbBytes(rgb):
    return chr(rgb.r) + chr(rgb.g) + chr(rgb.b)

def isPow2(x):
    return x in ( (1 << i) for i in xrange(32) )

def powUp(x):
    x -= 1
    x |= x >> 1
    x |= x >> 2
    x |= x >> 4
    x |= x >> 8
    x |= x >> 16
    return x + 1


def picFromTexture(tex):
    width = tex.width
    height = tex.height
    pixels = None
    mask = None

    rows = []
    maskrows = []
    for y in xrange(height):
        rows.append( "".join( (col[y] for col in tex.columns) ) )
        maskrows.append( "".join( (col[y] for col in tex.masks) ) )

    allpixels = "".join(rows)
    pixels = [rgbForPixel(p) for p in allpixels]

    allmask = "".join(maskrows)
    if "\x00" not in allmask:
        # all pixels opaque; no mask
        mask = None
    else:
        mask = [ord(m) for m in allmask]

    ret = Pic()
    ret.width = width
    ret.height = height
    ret.pixels = pixels
    ret.mask = mask

    return ret


def savePicSimple(path, pic):
    with open(path, "wb") as fp:
        fp.write(struct.pack("<I", pic.width))
        fp.write(struct.pack("<I", pic.height))
        if not pic.mask:
            fp.write("".join((rgbBytes(rgb) for rgb in pic.pixels)))
        else:
            # if there's a mask, replace transparent pixels w/ cyan
            rgbs = []
            for idx in xrange(len(pic.pixels)):
                if pic.mask[idx]:
                    rgbs.append(rgbBytes(pic.pixels[idx]))
                else:
                    rgbs.append("\x00\xff\xff")
            fp.write("".join(rgbs))


def picScaledUpToPow2(pic):
    new_width = powUp(pic.width)
    new_height = powUp(pic.height)
    new_pixels = None
    new_mask = None

    if (new_width, new_height) == (pic.width, pic.height):
        return pic

    # uses point sampling

    FRACBITS = 20

    xstep = pic.width * (1 << FRACBITS) / new_width
    xmap = [(idx * xstep) >> FRACBITS for idx in xrange(new_width)]

    ystep = pic.height * (1 << FRACBITS) / new_height
    ymap = [(idx * ystep) >> FRACBITS for idx in xrange(new_height)]

    new_pixels = [0] * (new_width * new_height)
    for y in xrange(new_height):
        for x in xrange(new_width):
            new_pixels[y * new_width + x] = pic.pixels[ymap[y] * pic.width + xmap[x]]

    if pic.mask:
        new_mask = [0] * (new_width * new_height)
        for y in xrange(new_height):
            for x in xrange(new_width):
                new_mask[y * new_width + x] = pic.mask[ymap[y] * pic.width + xmap[x]]

    ret = Pic()
    ret.width = new_width
    ret.height = new_height
    ret.pixels = new_pixels
    ret.mask = new_mask

    return ret


def picScaledDown(pic, new_width, new_height):
    """
    Must be a power-of-2 downscaling.
    Note the mask values are assumed to be floats, 0.0=transparent
    1.0=opaque.
    """

    if 1 in (pic.width, pic.height):
        # too small; can't quarter
        return pic

    new_pixels = []
    new_mask = None

    chunk_width = pic.width / new_width
    chunk_height = pic.height / new_height
    chunk_total = chunk_width * chunk_height

    oldy = 0
    for y in xrange(new_height):
        oldx = 0
        for x in xrange(new_width):
            new_pixels.append(pic.chunkColor(oldx, oldy, chunk_width, chunk_height))
            oldx += chunk_width
        oldy += chunk_height

    if pic.mask:
        new_mask = []

        oldy = 0
        for y in xrange(new_height):
            oldx = 0
            for x in xrange(new_width):
                new_mask.append(pic.chunkOpacity(oldx, oldy, chunk_width, chunk_height))
                oldx += chunk_width
            oldy += chunk_height

    ret = Pic()
    ret.width = new_width
    ret.height = new_height
    ret.pixels = new_pixels
    ret.mask = new_mask

    return ret


def compressMask(mask):
    bytes_needed = (len(mask) + 7) / 8
    bytes_ = []

    idx = 0
    while idx < len(mask):
        run = mask[idx:idx + 8]
        run.extend([0] * (8 - len(run)))

        val = 0x0
        for bit in xrange(8):
            val |= run[bit] << bit

        bytes_.append(val)

        idx += 8

    return "".join((chr(v) for v in bytes_))


def saveMipsForTexDef(texdef, levels):
    ident = "TEXMIPS\x00"
    name = texdef.name
    width = levels[0].width
    height = levels[0].height
    original_width = texdef.width
    original_height = texdef.height

    masked = bool(levels[0].mask and 0 in levels[0].mask)

    with open(name + os.path.extsep + "tex", "wb") as fp:
        fp.write(ident)
        fp.write(struct.pack("<H", OUT_VERSION))
        fp.write(wad.wadifyString(name))
        fp.write(struct.pack("<H", width))
        fp.write(struct.pack("<H", height))
        fp.write(struct.pack("<H", original_width))
        fp.write(struct.pack("<H", original_height))
        fp.write(struct.pack("<H", masked))

        # will come back here after we know the data size
        mipdatalen_pos = fp.tell()
        fp.write("\x00\x00\x00\x00")

        mipdata_start = fp.tell()

        for level in levels:
            fp.write("".join((rgbBytes(rgb) for rgb in level.pixels)))

            if masked:
                fp.write(compressMask(level.mask))

        mipdata_end = fp.tell()

        fp.seek(mipdatalen_pos)
        fp.write(struct.pack("<I", mipdata_end - mipdata_start))

    print "saved", name + os.path.extsep + "tex"


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print "usage: %s <wad> <texture1> <texture2> ..." % sys.argv[0]
        sys.exit(0)

    texutil.loadFromWad(sys.argv[1])

    if len(sys.argv) == 2:
        for texdef in texutil.texturedefs:
            print "%s (%dx%d)" % (texdef.name, texdef.width, texdef.height)
        sys.exit(0)

    if sys.argv[2] == "all":
        texnames = [texdef.name for texdef in texutil.texturedefs]
    else:
        texnames = sys.argv[2:]

    for name in texnames:
        texdef = texutil.findTextureDef(name)
        pic = picFromTexture(texutil.getTexture(texdef))

        # convert the 1 or 0 integer values to floats as we'll be doing
        # some averaging and want fractional parts
        if pic.mask:
            for idx in xrange(len(pic.mask)):
                pic.mask[idx] = float(pic.mask[idx])

        levels = [picScaledUpToPow2(pic)]
        for i in xrange(1, OUT_NUMLEVELS):
            if 1 in (levels[-1].width, levels[-1].height):
                break

            # Note that we're always scaling down from the original full
            # size image rather than scaling down the most recently
            # scaled level. This is to avoid some color loss.
            levels.append(picScaledDown(levels[0], levels[-1].width / 2, levels[-1].height / 2))

        # convert masks back to 1,0
        # for opacity values < 0.5, consider transparent
        for level in levels:
            if level.mask:
                level.mask = [int(round(val)) for val in level.mask]

        saveMipsForTexDef(texdef, levels)
