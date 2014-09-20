#!/usr/bin/env python3

import sys
import os
import struct
import collections

import wad
import texutil

PALNUM = 0

_maplumps = [   "THINGS", \
                "LINEDEFS", \
                "SIDEDEFS", \
                "VERTEXES", \
                "SEGS", \
                "SSECTORS", \
                "NODES", \
                "SECTORS", \
                "REJECT", \
                "BLOCKMAP" ]

THINGS_FILE = "THINGS%stxt" % os.path.extsep
VERTEXES_FILE = "VERTEXES%stxt" % os.path.extsep
LINEDEFS_FILE = "LINEDEFS%stxt" % os.path.extsep
SIDEDEFS_FILE = "SIDEDEFS%stxt" % os.path.extsep
SECTORS_FILE = "SECTORS%stxt" % os.path.extsep


def _iterLumpsForMap(w, mapname):
    idx = 0
    while idx < len(w.lumps):
        if w.lumps[idx].name == mapname:
            idx += 1
            found = False
            while idx < len(w.lumps) and w.lumps[idx].name in _maplumps:
                found = True
                yield w.lumps[idx]
                idx += 1
            if found:
                return
        else:
            idx += 1
    raise LookupError("cannot find map \"%s\"" % mapname)


def _dumpTHINGS(destdir, raw):
    path = os.path.join(destdir, THINGS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx x y angle type options\n")
        for idx, traw in enumerate(wad.iterChopBytes(raw, 10)):
            x, y, angle, type_, options = struct.unpack("<hhhhh", traw)
            fp.write("%d %d %d %d %d 0x%x\n" % (idx, x, y, angle, type_, options))


def _dumpVERTEXES(destdir, raw):
    path = os.path.join(destdir, VERTEXES_FILE)
    with open(path, "wt") as fp:
        fp.write("idx x y\n")
        for idx, vraw in enumerate(wad.iterChopBytes(raw, 4)):
            x, y = struct.unpack("<hh", vraw)
            fp.write("%d %d %d\n" % (idx, x, y))


def _dumpLINEDEFS(destdir, raw):
    path = os.path.join(destdir, LINEDEFS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx v1 v2 flags special tag side1 side2\n")
        for idx, lraw in enumerate(wad.iterChopBytes(raw, 14)):
            v1, v2, flags, special, tag, sidenum0, sidenum1 = struct.unpack("<hhhhhhh", lraw)
            fp.write("%d %d %d 0x%x %d %d %d %d\n" % (idx, v1, v2, flags, special, tag, sidenum0, sidenum1))


def _dumpSIDEDEFS(destdir, raw):
    path = os.path.join(destdir, SIDEDEFS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx x_shift y_shift uppertexture lowertexture middletexture sector\n")
        for idx, sraw in enumerate(wad.iterChopBytes(raw, 30)):
            xoff, yoff, toptex, bottomtex, middletex, sector = struct.unpack("<hh8s8s8sh", sraw)
            fp.write("%d %d %d \"%s\" \"%s\" \"%s\" %d\n" % (idx, xoff, yoff, wad.wadBytesToString(toptex), wad.wadBytesToString(bottomtex), wad.wadBytesToString(middletex), sector))


def _dumpSECTORS(destdir, raw):
    path = os.path.join(destdir, SECTORS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx floor ceiling floortexture ceilingtexture light special tag\n")
        for idx, sraw in enumerate(wad.iterChopBytes(raw, 26)):
            f_height, c_height, f_tex, c_tex, light, special, tag = struct.unpack("<hh8s8shhh", sraw)
            fp.write("%d %d %d \"%s\" \"%s\" %d %d %d\n" % (idx, f_height, c_height, wad.wadBytesToString(f_tex), wad.wadBytesToString(c_tex), light, special, tag))


def _mkdir(path):
    if not os.path.isdir(path):
        if os.path.isfile(path):
            raise ValueError("map file \"%s\" already exists" % path)
        os.mkdir(path)


def _dumpTextures(destdir, sidedefs_raw):
    texturesdir = os.path.join(destdir, "textures")
    _mkdir(texturesdir)

    all_used = set()

    for idx, sraw in enumerate(wad.iterChopBytes(sidedefs_raw, 30)):
        xoff, yoff, toptex, bottomtex, middletex, sector = struct.unpack("<hh8s8s8sh", sraw)
        toptex = wad.wadBytesToString(toptex)
        bottomtex = wad.wadBytesToString(bottomtex)
        middletex = wad.wadBytesToString(middletex)

        all_used.add(toptex)
        all_used.add(bottomtex)
        all_used.add(middletex)

    for name in all_used:
        if not name.strip() or name.strip() == "-":
            continue

        outpath = os.path.join(texturesdir, "%s%srgb" % (name, os.path.extsep))

        texdef = texutil.findTextureDef(name)
        tex = texutil.getTexture(texdef)

        rows = []
        maskrows = []
        for y in range(tex.height):
            rows.append( bytes(col[y] for col in tex.columns) )
            maskrows.append( bytes(col[y] for col in tex.masks) )

        allpixels = b"".join(rows)
        rgbs = b"".join( (texutil.palettes[PALNUM][pix] for pix in allpixels) )

        allmask = b"".join(maskrows)
        if b"\x00" not in allmask:
            # all pixels opaque; no mask
            mask = None
        else:
            mask = allmask

        with open(outpath, "wb") as fp:
            fp.write(struct.pack("<I", tex.width))
            fp.write(struct.pack("<I", tex.height))
            fp.write(rgbs)
            if mask:
                fp.write(mask)


def _dumpFlats(destdir, sectors_raw):
    flatsdir = os.path.join(destdir, "flats")
    _mkdir(flatsdir)

    all_used = set()

    for idx, sraw in enumerate(wad.iterChopBytes(sectors_raw, 26)):
        f_height, c_height, f_tex, c_tex, light, special, tag = struct.unpack("<hh8s8shhh", sraw)
        f_tex = wad.wadBytesToString(f_tex)
        c_tex = wad.wadBytesToString(c_tex)

        all_used.add(f_tex)
        all_used.add(c_tex)

    for name in all_used:
        outpath = os.path.join(flatsdir, "%s%srgb" % (name, os.path.extsep))

        if name == "F_SKY1":
            # Heretic's sky flat doesn't have 4096 bytes. It's used as a
            # place-holder anyways so it doesn't matter what we fill it
            # with.
            pixels = b"\xfb" * (64 * 64)
        else:
            pixels = texutil.getFlat(name)

        rgbs = b"".join( (texutil.palettes[PALNUM][pix] for pix in pixels) )

        with open(outpath, "wb") as fp:
            fp.write(struct.pack("<I", 64))
            fp.write(struct.pack("<I", 64))
            fp.write(rgbs)


def _dumpMap(w, mapname, lumps):
    _mkdir(mapname)

    dumpout = {}
    dumpout["THINGS"] = _dumpTHINGS
    dumpout["VERTEXES"] = _dumpVERTEXES
    dumpout["LINEDEFS"] = _dumpLINEDEFS
    dumpout["SIDEDEFS"] = _dumpSIDEDEFS
    dumpout["SECTORS"] = _dumpSECTORS

    for l in lumps:
        if l.name in dumpout:
            lumpraw = w.readLump(l)

            dumpout[l.name](mapname, lumpraw)

            if l.name == "SIDEDEFS":
                _dumpTextures(mapname, lumpraw)
            elif l.name == "SECTORS":
                _dumpFlats(mapname, lumpraw)


def main(argv):
    if len(argv) < 3:
        print("usage: %s <wad> mapname" % argv[0])
        sys.exit(0)

    w = wad.Wad(argv[1])

    texutil.loadFromWad(w)

    for mapname in argv[2:]:
        mapname = mapname.upper()
        _dumpMap(w, mapname, _iterLumpsForMap(w, mapname))

    texutil.clear()


def orderedObjs(objs):
    """
    For objs loaded from a list file, assuming each entry has an .idx
    attribute, simply put them into a dict for key'ing by the .idx
    """

    return collections.OrderedDict( ((o.idx, o) for o in objs) )


def loadObjList(path):
    ret = []

    with open(path, "rt") as fp:
        hdr = None
        for line in fp:
            line = line.strip()
            if not line:
                continue

            hdr = line.split()
            break

        if not hdr:
            raise ValueError("no header in \"%s\"" % path)

        class Obj(object):
            pass
        for attr in hdr:
            setattr(Obj, attr, None)

        for line in fp:
            line = line.strip()
            if not line:
                continue

            items = line.split()
            if len(items) != len(hdr):
                raise ValueError("invalid elements in line \"%s\"" % line)

            obj = Obj()
            for idx, str_ in enumerate(items):
                if str_.startswith("\""):
                    val = str_.strip("\"")
                elif "." in str_:
                    val = float(str_)
                else:
                    val = int(str_, 0)
                setattr(obj, hdr[idx], val)
            ret.append(obj)

    return ret

def loadTextTHINGS(mapdir):
    return loadObjList(os.path.join(mapdir, THINGS_FILE))

def loadTextVERTEXES(mapdir):
    return loadObjList(os.path.join(mapdir, VERTEXES_FILE))

def loadTextLINEDEFS(mapdir):
    return loadObjList(os.path.join(mapdir, LINEDEFS_FILE))

def loadTextSIDEDEFS(mapdir):
    return loadObjList(os.path.join(mapdir, SIDEDEFS_FILE))

def loadTextSECTORS(mapdir):
    return loadObjList(os.path.join(mapdir, SECTORS_FILE))


if __name__ == "__main__":
    main(sys.argv)
