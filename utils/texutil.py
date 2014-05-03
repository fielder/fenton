import collections
import struct

import wad


class Texture(object):
    name = ""
    width = 0
    height = 0
    columns = None # list of strings, each <height> bytes long
    masks = None # list of strings indicating if a pixel is
                 # transparent ("\x00"), or opaque ("\x01")
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
    pixels = None


wadfile = None
texturedefs = None
pnames = None
texturelists = None
playpal = None
palettes = None
patches = None


def clear():
    global wadfile
    global texturedefs
    global pnames
    global texturelists
    global playpal
    global palettes
    global patches

    if wadfile:
        wadfile.close()
        wadfile = None

    texturedefs = None
    pnames = None
    texturelists = None
    playpal = None
    palettes = None
    patches = None


def loadFromWad(w):
    global wadfile

    if isinstance(w, basestring):
        w = wad.Wad(w)
    elif isinstance(w, wad.Wad):
        pass
    else:
        raise ValueError("cannot load wad %s" % w)

    clear()

    wadfile = w

    loadPalettes()
    loadPNAMES()
    loadPatches()
    loadTextureList(1)
    loadTextureList(2)
    loadTextureDefs()


def loadPalettes():
    global playpal
    global palettes

    playpal = wadfile.readLump("PLAYPAL")
    palettes = []
    for rawpal in wad.chopBytes(playpal, 768):
        pal = { chr(i): rawpal[i * 3: i * 3 + 3] for i in xrange(256) }
        palettes.append(pal)


def loadPNAMES():
    global pnames

    raw = wadfile.readLump("PNAMES")
    cnt, = struct.unpack("<i", raw[:4])

    pnames = []
    for name8 in wad.chopBytes(raw, 8, count=cnt, start_offset=4):
        pnames.append(wad.pythonifyString(name8))


def loadTextureList(texlistnum):
    global texturelists

    if texturelists is None:
        texturelists = collections.OrderedDict()

    texlistname = "TEXTURE%d" % texlistnum

    tex_name_to_fileofs = {}

    lump = wadfile.lumps[wadfile.lump_name_to_num[texlistname]]
    raw = wadfile.readLump(lump)
    cnt, = struct.unpack("<i", raw[:4])
    for entry in wad.chopBytes(raw, 4, count=cnt, start_offset=4):
        texdef_offset = lump.filepos + struct.unpack("<i", entry)[0]
        name = wad.pythonifyString(wadfile.readBytes(texdef_offset, 8))
        tex_name_to_fileofs[name] = texdef_offset

    texturelists[texlistname] = tex_name_to_fileofs


def loadTextureDefs():
    global texturedefs

    texturedefs = []
    for texlist in texturelists.itervalues():
        for fileofs in texlist.itervalues():
            texturedefs.append(getTextureDef(fileofs))


def getTextureDef(fileofs):
    name = wad.pythonifyString(wadfile.readBytes(fileofs, 8))
    width, = struct.unpack("<h", wadfile.readBytes(fileofs + 12, 2))
    height, = struct.unpack("<h", wadfile.readBytes(fileofs + 14, 2))
    numpatches, = struct.unpack("<h", wadfile.readBytes(fileofs + 20, 2))

    patchdefs = []
    patchdefsraw = wadfile.readBytes(fileofs + 22, numpatches * 10)
    for raw in wad.chopBytes(patchdefsraw, 10, count=numpatches):
        tdp = TextureDefPatch()
        tdp.originx, = struct.unpack("<h", raw[0:2])
        tdp.originy, = struct.unpack("<h", raw[2:4])
        tdp.patchnum, = struct.unpack("<h", raw[4:6])
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
    raise LookupError("no texture def \"%s\"" % name)


def loadPatches():
    global patches

    patches = {}
    for name in pnames:
        raw = wadfile.readLump(name)

        w, = struct.unpack("<h", raw[0:2])
        h, = struct.unpack("<h", raw[2:4])
        left_off, = struct.unpack("<h", raw[4:6])
        top_off, = struct.unpack("<h", raw[6:8])

        posts = []
        for columnofsraw in wad.chopBytes(raw, 4, count=w, start_offset=8):
            columnofs, = struct.unpack("<i", columnofsraw)

            colposts = []
            idx = columnofs
            while raw[idx] != "\xff":
                topdelta = ord(raw[idx])
                length = ord(raw[idx + 1])
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
            for idx in xrange(count):
                col[position + idx] = post.pixels[idx]
                mask[position + idx] = "\x01"


def getTexture(texdef):
    cols = [["\x00"] * texdef.height for x in xrange(texdef.width)]
    masks = [["\x00"] * texdef.height for x in xrange(texdef.width)]

    for tdp in texdef.patches:
        patch = patches[pnames[tdp.patchnum]]

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
    ret.columns = [ "".join(col) for col in cols ]
    ret.masks = [ "".join(col) for col in masks ]

    return ret


def getFlat(name):
    start = wadfile.lump_name_to_num["F_START"]
    end = wadfile.lump_name_to_num["F_END"]
    for idx in xrange(start + 1, end):
        lump = wadfile.lumps[idx]
        if lump.name == name and lump.size == 4096:
            return wadfile.readLump(lump)
    raise LookupError("failed finding flat \"%s\"" % name)


if __name__ == "__main__":
    loadFromWad("doom.wad")
    textures = [getTexture(td) for td in texturedefs]
    print "%d textures loaded" % len(textures)