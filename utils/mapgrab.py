#!/usr/bin/env python3

import sys
import os
import struct

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

THINGS_FILE = "THINGS{}txt".format(os.path.extsep)
VERTEXES_FILE = "VERTEXES{}txt".format(os.path.extsep)
LINEDEFS_FILE = "LINEDEFS{}txt".format(os.path.extsep)
SIDEDEFS_FILE = "SIDEDEFS{}txt".format(os.path.extsep)
SECTORS_FILE = "SECTORS{}txt".format(os.path.extsep)


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
    raise LookupError("cannot find map \"{}\"".format(mapname))


def _dumpTHINGS(destdir, raw):
    path = os.path.join(destdir, THINGS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx x y angle type options\n")
        for idx, traw in enumerate(wad.iterChopBytes(raw, 10)):
            x, y, angle, type_, options = struct.unpack("<hhhhh", traw)
            fp.write("{} {} {} {} {} 0x{:x}\n".format(idx, x, y, angle, type_, options))


def _dumpVERTEXES(destdir, raw):
    path = os.path.join(destdir, VERTEXES_FILE)
    with open(path, "wt") as fp:
        fp.write("idx x y\n")
        for idx, vraw in enumerate(wad.iterChopBytes(raw, 4)):
            x, y = struct.unpack("<hh", vraw)
            fp.write("{} {} {}\n".format(idx, x, y))


def _dumpLINEDEFS(destdir, raw):
    path = os.path.join(destdir, LINEDEFS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx v1 v2 flags special tag side1 side2\n")
        for idx, lraw in enumerate(wad.iterChopBytes(raw, 14)):
            v1, v2, flags, special, tag, sidenum0, sidenum1 = struct.unpack("<hhhhhhh", lraw)
            fp.write("{} {} {} 0x{:x} {} {} {} {}\n".format(idx, v1, v2, flags, special, tag, sidenum0, sidenum1))


def _dumpSIDEDEFS(destdir, raw):
    path = os.path.join(destdir, SIDEDEFS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx x_shift y_shift uppertexture lowertexture middletexture sector\n")
        for idx, sraw in enumerate(wad.iterChopBytes(raw, 30)):
            xoff, yoff, toptex, bottomtex, middletex, sector = struct.unpack("<hh8s8s8sh", sraw)
            fp.write("{} {} {} \"{}\" \"{}\" \"{}\" {}\n".format(idx, xoff, yoff, wad.wadBytesToString(toptex), wad.wadBytesToString(bottomtex), wad.wadBytesToString(middletex), sector))


def _dumpSECTORS(destdir, raw):
    path = os.path.join(destdir, SECTORS_FILE)
    with open(path, "wt") as fp:
        fp.write("idx floor ceiling floortexture ceilingtexture light special tag\n")
        for idx, sraw in enumerate(wad.iterChopBytes(raw, 26)):
            f_height, c_height, f_tex, c_tex, light, special, tag = struct.unpack("<hh8s8shhh", sraw)
            fp.write("{} {} {} \"{}\" \"{}\" {} {} {}\n".format(idx, f_height, c_height, wad.wadBytesToString(f_tex), wad.wadBytesToString(c_tex), light, special, tag))


def _mkdir(path):
    if not os.path.isdir(path):
        if os.path.isfile(path):
            raise ValueError("map file \"{}\" already exists".format(path))
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

        outpath = os.path.join(texturesdir, "{}{}rgb".format(name, os.path.extsep))

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
            # transparent pixels stay 0
            # opaque pixels have alpha of 255
            mask = bytes( (m * 0xff for m in allmask) )

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
        outpath = os.path.join(flatsdir, "{}{}rgb".format(name, os.path.extsep))

        if name == "F_SKY1":
            # Heretic's sky flat doesn't have 4096 bytes. It's used as a
            # place-holder anyways so it doesn't matter what we fill it
            # with.
            pixels = b"\xfb" * (64 * 64)
        else:
            try:
                pixels = texutil.getFlat(name)
            except LookupError as e:
                print("Warning: failed fetching a flat ({})".format(e))
                continue

        rgbs = b"".join( (texutil.palettes[PALNUM][pix] for pix in pixels) )

        with open(outpath, "wb") as fp:
            fp.write(struct.pack("<I", 64))
            fp.write(struct.pack("<I", 64))
            fp.write(rgbs)
            # no alpha in flats so no alpha mask written


def _dumpMap(w, mapname, lumps):
    # run the iterator to die now if the map can't be found so we don't
    # make an empty directory
    lumps = list(lumps)

    _mkdir(mapname)

    for l in lumps:

        if l.name == "THINGS":
            _dumpTHINGS(mapname, w.readLump(l))

        elif l.name == "VERTEXES":
            _dumpVERTEXES(mapname, w.readLump(l))

        elif l.name == "LINEDEFS":
            _dumpLINEDEFS(mapname, w.readLump(l))

        elif l.name == "SIDEDEFS":
            lumpraw = w.readLump(l)
            _dumpSIDEDEFS(mapname, lumpraw)
            _dumpTextures(mapname, lumpraw)

        elif l.name == "SECTORS":
            lumpraw = w.readLump(l)
            _dumpSECTORS(mapname, lumpraw)
            _dumpFlats(mapname, lumpraw)

        else:
            # map lump we don't care about... for now
            pass


def main(argv):
    if len(argv) < 3:
        print("usage: {} <wad> mapname".format(argv[0]))
        sys.exit(0)

    w = wad.Wad(argv[1])

    texutil.loadFromWad(w)

    for mapname in argv[2:]:
        mapname = mapname.upper()
        _dumpMap(w, mapname, _iterLumpsForMap(w, mapname))

    texutil.clear()


if __name__ == "__main__":
    main(sys.argv)
