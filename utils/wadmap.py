#!/usr/bin/env python3

import collections
import struct

import wad

_maplumps = [ "THINGS", \
              "LINEDEFS", \
              "SIDEDEFS", \
              "VERTEXES", \
              "SEGS", \
              "SSECTORS", \
              "NODES", \
              "SECTORS", \
              "REJECT", \
              "BLOCKMAP" ]

wadvertex = collections.namedtuple("wadvertex",
    "x y")

wadlinedef = collections.namedtuple("wadlinedef",
    "v1 v2 flags special tag sidenum0 sidenum1")

wadsidedef = collections.namedtuple("wadsidedef",
    "xoff yoff toptex bottex midtex sector")

wadsector = collections.namedtuple("wadsector",
    "floor ceiling floortex ceilingtex light special tag")

wadthing = collections.namedtuple("wadthing",
    "x y angle type options")


def iterVertexes(raw):
    for r in wad.iterChopBytes(raw, 4):
        x, y = struct.unpack("<hh", r)
        yield wadvertex(x, y)


def iterLinedefs(raw):
    for r in wad.iterChopBytes(raw, 14):
        v1, v2, flags, special, tag, sidenum0, sidenum1 = struct.unpack("<hhhhhhh", r)
        yield wadlinedef(v1,
                         v2,
                         flags,
                         special,
                         tag,
                         sidenum0,
                         sidenum1)


def iterSidedefs(raw):
    for r in wad.iterChopBytes(raw, 30):
        xoff, yoff, toptex, bottex, midtex, sector = struct.unpack("<hh8s8s8sh", r)
        yield wadsidedef(xoff,
                         yoff,
                         wad.wadBytesToString(toptex),
                         wad.wadBytesToString(bottex),
                         wad.wadBytesToString(midtex),
                         sector)


def iterSectors(raw):
    for r in wad.iterChopBytes(raw, 26):
         f, c, ftex, ctex, light, special, tag = struct.unpack("<hh8s8shhh", r)
         yield wadsector(f,
                         c,
                         wad.wadBytesToString(ftex),
                         wad.wadBytesToString(ctex),
                         light,
                         special,
                         tag)


def iterThings(raw):
    for r in wad.iterChopBytes(raw, 10):
         x, y, angle, type_, options = struct.unpack("<hhhhh", r)
         yield wadthing(x, y, angle, type_, options)


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
