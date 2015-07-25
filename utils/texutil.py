#!/usr/bin/env python3

import collections
import struct

import wad


class Texture(object):
    name = ""
    width = 0
    height = 0
    columns = None # list of bytes, each <height> bytes long
    masks = None # list of bytes indicating if a pixel is
                 # transparent (0), or opaque (1)
                 # masks is None if fully opaque

class TextureDef(object):
    name = ""
    width = 0
    height = 0
    patches = None

class TextureDefPatch(object):
    originx = 0
    originy = 0
    patchnum = 0 # index in the PNAMES list of strings
    stepdir = 0 # unused
    colormap = 0 # unused

class Patch(object):
    width = 0
    height = 0
    leftoffset = 0 # pixels to the left of origin
    topoffset = 0 # pixels below the origin
    posts = None

class Post(object):
    topdelta = 0
    pixels = None # bytes string


wadfile = None
texturedefs = None
pnames = None
texturelists = None
palettes = None
patches = None


def clear():
    global wadfile
    global texturedefs
    global pnames
    global texturelists
    global palettes
    global patches

    if wadfile:
        wadfile.close()
        wadfile = None

    texturedefs = None
    pnames = None
    texturelists = None
    palettes = None
    patches = None


def loadFromWad(w):
    global wadfile

    if isinstance(w, str):
        w = wad.Wad(w)
    elif isinstance(w, wad.Wad):
        pass
    else:
        raise ValueError("cannot load wad {}".format(w))

    clear()

    wadfile = w

    _loadPalettes()
    _loadPNAMES()
    _loadTextureList("TEXTURE1")
    if "TEXTURE2" in wadfile.lump_names:
        # shareware wad won't have 2nd texture set
        _loadTextureList("TEXTURE2")
    _loadTextureDefs()


def _loadPalettes():
    global palettes

    playpal = wadfile.readLump("PLAYPAL")

    palettes = []
    for rawpal in wad.iterChopBytes(playpal, 768):
        pal = [rawpal[i * 3: i * 3 + 3] for i in range(256)]
        palettes.append(pal)


def _loadPNAMES():
    global pnames

    raw = wadfile.readLump("PNAMES")
    cnt, = struct.unpack("<i", raw[:4])

    # The PNAMES lump is just a list of strings telling the names of
    # patches. Textures refer to patches by number, not name. This
    # number is an index into the PNAMES list.

    pnames = []
    for name8 in wad.iterChopBytes(raw, 8, count=cnt, start_offset=4):
        pnames.append(wad.wadBytesToString(name8))


def _loadTextureList(texlistname):
    global texturelists

    if texturelists is None:
        texturelists = collections.OrderedDict()

    tex_name_to_fileofs = {}

    lump = wadfile.lumps[wadfile.lump_name_to_num[texlistname]]
    raw = wadfile.readLump(lump)
    cnt, = struct.unpack("<i", raw[:4])
    for entry in wad.iterChopBytes(raw, 4, count=cnt, start_offset=4):
        texdef_offset = lump.filepos + struct.unpack("<i", entry)[0]
        name = wad.wadBytesToString(wadfile.readBytes(texdef_offset, 8))
        tex_name_to_fileofs[name] = texdef_offset

    texturelists[texlistname] = tex_name_to_fileofs


def _loadTextureDefs():
    global texturedefs

    texturedefs = []
    for texlist in texturelists.values():
        for texdef_offset in texlist.values():
            texturedefs.append(_getTextureDef(texdef_offset))


def _getTextureDef(fileofs):
    name = wad.wadBytesToString(wadfile.readBytes(fileofs, 8))
    # bytes 8-11 are unused
    width, = struct.unpack("<h", wadfile.readBytes(fileofs + 12, 2))
    height, = struct.unpack("<h", wadfile.readBytes(fileofs + 14, 2))
    # bytes 16-19 are unused
    numpatches, = struct.unpack("<h", wadfile.readBytes(fileofs + 20, 2))

    patchdefs = []
    patchdefsraw = wadfile.readBytes(fileofs + 22, numpatches * 10)
    for raw in wad.iterChopBytes(patchdefsraw, 10, count=numpatches):
        tdp = TextureDefPatch()
        tdp.originx, = struct.unpack("<h", raw[0:2])
        tdp.originy, = struct.unpack("<h", raw[2:4])
        tdp.patchnum, = struct.unpack("<h", raw[4:6])
        # bytes 6-9 are unused
        patchdefs.append(tdp)

    ret = TextureDef()
    ret.name = name
    ret.width = width
    ret.height = height
    ret.patches = patchdefs

    return ret


def findTextureDef(name):
    name = name.upper()
    for td in texturedefs:
        if td.name == name:
            return td
    raise LookupError("no texture def \"{}\"".format(name))


def _getPatch(name):
    global patches

    if patches is None:
        patches = {}

    if name not in patches:
        raw = wadfile.readLump(name)

        w, = struct.unpack("<h", raw[0:2])
        h, = struct.unpack("<h", raw[2:4])
        left_off, = struct.unpack("<h", raw[4:6])
        top_off, = struct.unpack("<h", raw[6:8])

        posts = []
        for columnofsraw in wad.iterChopBytes(raw, 4, count=w, start_offset=8):
            columnofs, = struct.unpack("<i", columnofsraw)

            colposts = []
            idx = columnofs
            while raw[idx] != 255:
                topdelta = raw[idx]
                length = raw[idx + 1]
                pixels = raw[idx + 3:idx + 3 + length]
                idx += length + 4

                post = Post()
                post.topdelta = topdelta
                post.pixels = pixels

                colposts.append(post)

            posts.append(colposts)

        p = Patch()
        p.width = w
        p.height = h
        p.leftoffset = left_off
        p.topoffset = top_off
        p.posts = posts

        patches[name] = p

    return patches[name]


def _paintColumn(col, mask, posts, originy):
    for post in posts:
        position = originy + post.topdelta
        count = len(post.pixels)

        if position < 0:
            count += position
            position = 0

        if position + count > len(col):
            count = len(col) - position

        if count > 0:
            for idx in range(count):
                col[position + idx] = post.pixels[idx]
                mask[position + idx] = 1


def getTexture(texdef):
    cols = [[0] * texdef.height for x in range(texdef.width)]
    masks = [[0] * texdef.height for x in range(texdef.width)]

    for tdp in texdef.patches:
        # Note we get and build patches on the fly rather than when
        # initially setting up the wad. Previously we loaded all patches
        # in the PNAMES lump. But, some wads have PNAMES entries that
        # are not used by any texture and also don't point to an actual
        # existing patch lump.
        patch = _getPatch(pnames[tdp.patchnum])

        x1 = tdp.originx
        x2 = x1 + patch.width

        x = max(0, x1)
        x2 = min(x2, texdef.width)

        while x < x2:
            posts = patch.posts[x - x1]
            _paintColumn(cols[x], masks[x], posts, tdp.originy)
            x += 1

    ret = Texture()
    ret.name = texdef.name
    ret.width = texdef.width
    ret.height = texdef.height
    ret.columns = [ bytes(col) for col in cols ]
    ret.masks = [ bytes(col) for col in masks ]

    return ret


def getFlat(name):
    start = wadfile.lump_name_to_num["F_START"]
    end = wadfile.lump_name_to_num["F_END"]
    for idx in range(start + 1, end):
        lump = wadfile.lumps[idx]
        if lump.name == name and lump.size == 4096:
            return wadfile.readLump(lump)
    raise LookupError("failed finding flat \"{}\"".format(name))


def textureToRGB(tex, palnum=0):
    rows = []
    maskrows = []
    for y in range(tex.height):
        rows.append( bytes(col[y] for col in tex.columns) )
        maskrows.append( bytes(col[y] for col in tex.masks) )

    allpixels = b"".join(rows)
    allmask = b"".join(maskrows)

    if b"\x00" not in allmask:
        # all pixels opaque; no alpha
        return (pixelsToRGB(allpixels, palnum=palnum), False)
    else:
        # transparency; tack on an alpha channel
        return (b"".join((palettes[palnum][allpixels[i]] + bytes([allmask[i] * 255]) for i in range(len(allpixels)))), True)


def pixelsToRGB(pixels, palnum=0):
    return b"".join( (palettes[palnum][pix] for pix in pixels) )


if __name__ == "__main__":
    import sys
    for path in sys.argv[1:]:
        loadFromWad(path)
        textures = [getTexture(td) for td in texturedefs]
        print("{} textures loaded in \"{}\"".format(len(textures), path))
        clear()
