import collections
import struct
import zlib

_png_signature = bytes((137, 80, 78, 71, 13, 10, 26, 10))


def _iterChop(sequence, chunksz):
    for idx in range(0, len(sequence), chunksz):
        yield sequence[idx : idx + chunksz]


def _mkChunk(type_, bytes_):
    body = type_.encode() + bytes_
    return  struct.pack(">I", len(bytes_)) + \
            body + \
            struct.pack(">I", zlib.crc32(body))


def _buildPalettized(pixels, width, height, paldict):
    """
    paldict is in the form { b"\\xRR\\xGG\\xBB" : pal_idx }
    """

    # convert rgb -> pal_idx
    idxpix = bytes((paldict[p] for p in _iterChop(pixels, 3)))

    # the png spec recommends to just filter rows with type 0 for
    # palettized images (no filtering)

    filtered = b"".join((b"\x00" + row for row in _iterChop(idxpix, width)))

    ihdr_dat = struct.pack(">IIBBBBB", width, height, 8, 3, 0, 0, 0)
    plte_dat = b"".join(paldict.keys())
    idat_dat = zlib.compress(filtered)

    ihdr_chunk = _mkChunk("IHDR", ihdr_dat)
    plte_chunk = _mkChunk("PLTE", plte_dat)
    idat_chunk = _mkChunk("IDAT", idat_dat)
    iend_chunk = _mkChunk("IEND", b"")

    return _png_signature + ihdr_chunk + plte_chunk + idat_chunk + iend_chunk


def _sub(a, b):
    return (a - b) % 256

def _up(pixels, width, bpp, i):
    if i < width * bpp:
        return 0
    return pixels[i - width * bpp]

def _left(pixels, bpp, i):
    if i < bpp:
        return 0
    return pixels[i - bpp]

def _filt0(pixels, width, bpp):
    return pixels

def _filt1(pixels, width, bpp):
    return bytes((_sub(pixels[i], _left(pixels, bpp, i)) for i in range(len(pixels))))

def _filt2(pixels, width, bpp): 
    return bytes((_sub(pixels[i], _up(pixels, width, bpp, i)) for i in range(len(pixels))))

#FIXME: :-(
def _filt3(pixels, width, bpp):
    return bytes((
        _sub(   pixels[i],
                int((_left(pixels, bpp, i) + _up(pixels, width, bpp, i)) / 2)
            )
        for i in range(len(pixels))))


def _filterRow(pixels, bpp):
    width = int(len(pixels) / bpp)

    # one entry per filter type; 5 total
    # keyed by the quality of that filter method; lower is better
    # { sum_of_filtered: (filter_type, filtered_data) }
    sums = collections.OrderedDict()

    # (type 0) unfiltered
#   enc = _filt0(pixels, width, bpp)
#   sums[sum(enc)] = (0, enc)

    # (type 1) pix[i] - pix[left]
#   enc = _filt1(pixels, width, bpp)
#   sums[sum(enc)] = (1, enc)

    # (type 2) pix[i] - pix[up]
#   enc = _filt2(pixels, width, bpp)
#   sums[sum(enc)] = (2, enc)

    # (type 3) pix[i] - floor((pix[left] + pix[up]) / 2)
    enc = _filt3(pixels, width, bpp)
    sums[sum(enc)] = (3, enc)

    #TODO: paeth

    type_, enc = sums[min(sums.keys())]
    return bytes([type_]) + enc


def _buildTrueColor(pixels, width, height, is_rgba):
    bpp = { False: 3, True: 4 }[is_rgba]
    imgtyp = { False: 2, True: 6 }[is_rgba]

    filtered = b"".join((_filterRow(r, bpp) for r in _iterChop(pixels, width * bpp)))

    ihdr_dat = struct.pack(">IIBBBBB", width, height, 8, imgtyp, 0, 0, 0)
    idat_dat = zlib.compress(filtered)

    ihdr_chunk = _mkChunk("IHDR", ihdr_dat)
    idat_chunk = _mkChunk("IDAT", idat_dat)
    iend_chunk = _mkChunk("IEND", b"")

    return _png_signature + ihdr_chunk + idat_chunk + iend_chunk


def buildPNG(pixels, width, height, is_rgba=False):
    """
    """

    if True: # rgb debug
        return _buildTrueColor(pixels, width, height, is_rgba)

    if not is_rgba:
        if (len(pixels) % 3) != 0:
            raise Exception("invalid pixels")

        # check if it can be written out as a palettized
        # image; but can't have alpha with palettized images

        # build up the palette; { rgb: index_in_palette, ... }
        pal = collections.OrderedDict()

        for p in _iterChop(pixels, 3):
            pal.setdefault(p, len(pal))
            if len(pal) > 256:
                # too many different colors for a palettized image
                break
        else:
            # <= 256 colors
            return _buildPalettized(pixels, width, height, pal)

    return _buildTrueColor(pixels, width, height, is_rgba)


def writePNG(path, pixels, width, height, is_rgba=False):
    """
    """

    with open(path, "wb") as fp:
        fp.write(buildPNG(pixels, width, height, is_rgba=is_rgba))


if __name__ == "__main__":
    #_test1sz = 2
    _test1sz = 31
    _test1 = ( \
        (b"\xff\xff\xff" * _test1sz) + \
        (b"\xff\x00\x00" * _test1sz) + \
        (b"\x00\xff\x00" * _test1sz) + \
        (b"\x00\x00\xff" * _test1sz) + \
        (b"\x00\x00\x00" * _test1sz) \
        ) * _test1sz
    writePNG("test1.png",_test1,_test1sz*5,_test1sz)

    _test2sz = 2
    #_test2sz = 7
    _test2 = ( \
        (b"\xff\xff\xff\xff" * _test2sz) + \
        (b"\xff\x00\x00\xff" * _test2sz) + \
        (b"\x00\xff\x00\xff" * _test2sz) + \
        (b"\x00\x00\xff\xff" * _test2sz) + \
        (b"\x00\x00\x00\xff" * _test2sz) \
        ) * _test2sz
    writePNG("test2.png",_test2,_test2sz*5,_test2sz,True)


# all values in network byte ordering
# chunk:
# - data len (uint)
# - type (4 ascii bytes, A-Z or a-z, non-terminated)
# - data bytes; can be 0
# - crc of type & data fields

# image types:
# x 0 greyscale
# - 2 truecolor
# - 3 palettized
# x 4 greyscale w/ alpha
# - 6 truecolor w/ alpha

# header chunk:
# - uint width
# - uint height
# - uchar bits per sample; will use 8
# - uchar color type; 2, 3, or 6
# - uchar compression method, always 0
# - uchar filter method, always 0
# - uchar interlace method, use 0, no interlacing

# palette chunk:
# - simple RGB triplets

# end chunk:
# - no data
