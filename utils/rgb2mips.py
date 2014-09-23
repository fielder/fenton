#!/usr/bin/env python3

import sys
import os
import struct
import collections

RGB = collections.namedtuple("RGB", "r g b")

OUT_VERSION = 1
OUT_NUMLEVELS = 4


class Pic(object):
    width = 0
    height = 0
    pixels = None # list of RGB's
    mask = None # list of floats, 0.0 for transparent pixel, 1.0 for opaque

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

        for cy in range(chunk_height):
            for cx in range(chunk_width):
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

        for cy in range(chunk_height):
            for cx in range(chunk_width):
                tot += self.maskval(x + cx, y + cy)

        return tot / (chunk_width * chunk_height)

    def pixelsAsBytes(self):
        return "".join( (rgbBytes(rgb) for rgb in self.pixels) )

    def maskAsBytes(self):
        if not self.mask:
            raise Exception("no mask")

        bytes_needed = (len(self.mask) + 7) / 8
        bytes_ = []

        idx = 0
        while idx < len(self.mask):
            run = self.mask[idx:idx + 8]
            run.extend([0.0] * (8 - len(run)))

            # convert float mask pixel translucency values to ints
            # for opacity values < 0.5, consider fully transparent
            irun = [int(round(val)) for val in run]

            val = 0x0
            for bit in range(8):
                val |= irun[bit] << bit

            bytes_.append(val)

            idx += 8

        return "".join((chr(b) for b in bytes_))


def powUp(x):
    x -= 1
    x |= x >> 1
    x |= x >> 2
    x |= x >> 4
    x |= x >> 8
    x |= x >> 16
    return x + 1


def rgbBytes(rgb):
    return chr(rgb.r) + chr(rgb.g) + chr(rgb.b)


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
    xmap = [(idx * xstep) >> FRACBITS for idx in range(new_width)]

    ystep = pic.height * (1 << FRACBITS) / new_height
    ymap = [(idx * ystep) >> FRACBITS for idx in range(new_height)]

    new_pixels = [0] * (new_width * new_height)
    for y in range(new_height):
        for x in range(new_width):
            new_pixels[y * new_width + x] = pic.pixels[ymap[y] * pic.width + xmap[x]]

    if pic.mask:
        new_mask = [0] * (new_width * new_height)
        for y in range(new_height):
            for x in range(new_width):
                new_mask[y * new_width + x] = pic.mask[ymap[y] * pic.width + xmap[x]]

    ret = Pic()
    ret.width = new_width
    ret.height = new_height
    ret.pixels = new_pixels
    ret.mask = new_mask

    return ret


def parseRGB(filedat):
    if len(filedat) <= 8:
        raise ValueError("not an image")

    width, = struct.unpack("<I", filedat[0:4])
    height, = struct.unpack("<I", filedat[4:8])
    if width <= 0 or width > 1024 or height <= 0 or height > 1024:
        raise ValueError("invalid size %dx%d" % (width, height))

    if len(filedat) != 8 + (width * height * 3):
        # there are extra bytes after the pixel data, should be the
        # alpha mask
        if len(filedat) != 8 + (width * height * 3) + (width * height):
            raise ValueError("invalid file size %d" % len(filedat))

    pixelstart = 8
    pixelend = pixelstart + width * height * 3
    pixelsraw = filedat[pixelstart:pixelend]
    pixels = [ RGB(pixelsraw[idx + 0], pixelsraw[idx + 1], pixelsraw[idx + 2]) for \
               idx in range(0, width * height * 3, 3) ]

    if pixelend < len(filedat):
        maskstart = pixelend
        maskend = maskstart + width * height
        maskraw = filedat[maskstart:maskend]
        mask = [float(mval != "\x00") for mval in maskraw]
    else:
        mask = None

    ret = Pic()
    ret.width = width
    ret.height = height
    ret.pixels = pixels
    ret.mask = mask

    return ret


def loadRGB(path):
    with open(path, "rb") as fp:
        try:
            return parseRGB(fp.read())
        except Exception as e:
            raise type(e)(str(e) + "; error while reading \"%s\"" % path)


def levelReduced(pic, new_width, new_height):
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
    for y in range(new_height):
        oldx = 0
        for x in range(new_width):
            new_pixels.append(pic.chunkColor(oldx, oldy, chunk_width, chunk_height))
            oldx += chunk_width
        oldy += chunk_height

    if pic.mask:
        new_mask = []

        oldy = 0
        for y in range(new_height):
            oldx = 0
            for x in range(new_width):
                new_mask.append(pic.chunkOpacity(oldx, oldy, chunk_width, chunk_height))
                oldx += chunk_width
            oldy += chunk_height

    ret = Pic()
    ret.width = new_width
    ret.height = new_height
    ret.pixels = new_pixels
    ret.mask = new_mask

    return ret


def saveMips(levels, name, original_width, original_height, path):
    name8 = name + "\x00" * (8 - len(name))

    masked = bool(levels[0].mask and 0.0 in levels[0].mask)

    with open(path, "wb") as fp:
        fp.write("TEXMIPS\x00")
        fp.write(struct.pack("<H", OUT_VERSION))
        fp.write(name8)
        fp.write(struct.pack("<H", levels[0].width))
        fp.write(struct.pack("<H", levels[0].height))
        fp.write(struct.pack("<H", original_width))
        fp.write(struct.pack("<H", original_height))
        fp.write(struct.pack("<H", masked))

        # will come back here after we know the data size
        mipdatalen_pos = fp.tell()
        fp.write("\x00\x00\x00\x00")

        mipdata_start = fp.tell()

        for level in levels:
            fp.write(level.pixelsAsBytes())
            if masked:
                fp.write(level.maskAsBytes())

        mipdata_end = fp.tell()

        fp.seek(mipdatalen_pos)
        fp.write(struct.pack("<I", mipdata_end - mipdata_start))


def convertRGB(path):
    dirs, filename = os.path.split(path)
    name, ext = os.path.splitext(filename)
    new_filename = "%s%s%s" % (name, os.path.extsep, "tex")
    new_path = os.path.join(dirs, new_filename)

    pic = loadRGB(path)

#FIXME
    return ########################################################################

    levels = [picScaledUpToPow2(pic)]
    for i in range(1, OUT_NUMLEVELS):
        if 1 in (levels[-1].width, levels[-1].height):
            # can't reduce anymore; done
            break

        # Note that we're always scaling down from the original full
        # size image rather than scaling down the most recently
        # scaled level. This is to avoid some color loss.
        levels.append(levelReduced(levels[0], levels[-1].width / 2, levels[-1].height / 2))

    saveMips(levels, name, pic.width, pic.height, new_path)


def main(argv):
    if len(argv) < 2:
        print("Convert .rgb images to mip-mapped texture images")
        print("usage: %s <rgb1> ..." % argv[0])
        sys.exit(0)

    for path in argv[1:]:
        convertRGB(path)


if __name__ == "__main__":
    main(sys.argv)
