#!/usr/bin/env python3

import collections
#import os
import struct

import wad
#import texutil

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


wadvertex = collections.namedtuple("vertex", "x y")

wadlinedef = collections.namedtuple("linedef", "v1 v2 flags special tag sidenum0 sidenum1")


def iterVertexes(raw):
    for r in wad.iterChopBytes(raw, 4):
        x, y = struct.unpack("<hh", r)
        yield wadvertex(x, y)


def iterLinedefs(raw):
    for r in wad.iterChopBytes(raw, 14):
        v1, v2, flags, special, tag, sidenum0, sidenum1 = struct.unpack("<hhhhhhh", r)
        yield wadlinedef(v1, v2, flags, special, tag, sidenum0, sidenum1)


def lumpForMap(w, mapname, lumpname):
    idx = 0
    while idx < len(w.lumps):
        if w.lumps[idx].name == mapname:
            idx += 1
            while idx < len(w.lumps) and w.lumps[idx].name in _maplumps:
                if w.lumps[idx].name == lumpname:
                    return w.readLump(w.lumps[idx])
                idx += 1
            break
        else:
            idx += 1
    raise LookupError("cannot find lump \"{}\" in map \"{}\"".format(lumpname, mapname))


#def _dumpTHINGS(destdir, raw):
#    path = os.path.join(destdir, THINGS_FILE)
#    with open(path, "wt") as fp:
#        fp.write("idx x y angle type options\n")
#        for idx, traw in enumerate(wad.iterChopBytes(raw, 10)):
#            x, y, angle, type_, options = struct.unpack("<hhhhh", traw)
#            fp.write("{} {} {} {} {} 0x{:x}\n".format(idx, x, y, angle, type_, options))


#def _dumpVERTEXES(destdir, raw):
#    path = os.path.join(destdir, VERTEXES_FILE)
#    with open(path, "wt") as fp:
#        fp.write("idx x y\n")
#        for idx, vraw in enumerate(wad.iterChopBytes(raw, 4)):
#            x, y = struct.unpack("<hh", vraw)
#            fp.write("{} {} {}\n".format(idx, x, y))


#def _dumpLINEDEFS(destdir, raw):
#    path = os.path.join(destdir, LINEDEFS_FILE)
#    with open(path, "wt") as fp:
#        fp.write("idx v1 v2 flags special tag side1 side2\n")
#        for idx, lraw in enumerate(wad.iterChopBytes(raw, 14)):
#            v1, v2, flags, special, tag, sidenum0, sidenum1 = struct.unpack("<hhhhhhh", lraw)
#            fp.write("{} {} {} 0x{:x} {} {} {} {}\n".format(idx, v1, v2, flags, special, tag, sidenum0, sidenum1))


#def _dumpSIDEDEFS(destdir, raw):
#    path = os.path.join(destdir, SIDEDEFS_FILE)
#    with open(path, "wt") as fp:
#        fp.write("idx x_shift y_shift uppertexture lowertexture middletexture sector\n")
#        for idx, sraw in enumerate(wad.iterChopBytes(raw, 30)):
#            xoff, yoff, toptex, bottomtex, middletex, sector = struct.unpack("<hh8s8s8sh", sraw)
#            fp.write("{} {} {} \"{}\" \"{}\" \"{}\" {}\n".format(idx, xoff, yoff, wad.wadBytesToString(toptex), wad.wadBytesToString(bottomtex), wad.wadBytesToString(middletex), sector))


#def _dumpSECTORS(destdir, raw):
#    path = os.path.join(destdir, SECTORS_FILE)
#    with open(path, "wt") as fp:
#        fp.write("idx floor ceiling floortexture ceilingtexture light special tag\n")
#        for idx, sraw in enumerate(wad.iterChopBytes(raw, 26)):
#            f_height, c_height, f_tex, c_tex, light, special, tag = struct.unpack("<hh8s8shhh", sraw)
#            fp.write("{} {} {} \"{}\" \"{}\" {} {} {}\n".format(idx, f_height, c_height, wad.wadBytesToString(f_tex), wad.wadBytesToString(c_tex), light, special, tag))
