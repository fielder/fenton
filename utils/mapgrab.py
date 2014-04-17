#!/usr/bin/env python

import sys
import os
import struct

import wad

_maplumps = ["THINGS", "LINEDEFS", "SIDEDEFS", \
             "VERTEXES", "SEGS", "SSECTORS", \
             "NODES", "SECTORS", "REJECT", \
             "BLOCKMAP"]


def _findMapLumps(w, mapname):
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
    path = os.path.join(destdir, "THINGS%stxt" % os.path.extsep)
    with open(path, "wt") as fp:
        fp.write("idx x y angle type options\n")
        for idx, traw in enumerate(wad.chopBytes(raw, 10)):
            x, y, angle, type_, options = struct.unpack("<hhhhh", traw)
            fp.write("%d %d %d %d %d 0x%x\n" % (idx, x, y, angle, type_, options))


def _dumpVERTEXES(destdir, raw):
    path = os.path.join(destdir, "VERTEXES%stxt" % os.path.extsep)
    with open(path, "wt") as fp:
        fp.write("idx x y\n")
        for idx, vraw in enumerate(wad.chopBytes(raw, 4)):
            x, y = struct.unpack("<hh", vraw)
            fp.write("%d %d %d\n" % (idx, x, y))


def _dumpLINEDEFS(destdir, raw):
    path = os.path.join(destdir, "LINEDEFS%stxt" % os.path.extsep)
    with open(path, "wt") as fp:
        fp.write("idx v1 v2 flags special tag side1 side2\n")
        for idx, lraw in enumerate(wad.chopBytes(raw, 14)):
            v1, v2, flags, special, tag, sidenum0, sidenum1 = struct.unpack("<hhhhhhh", lraw)
            fp.write("%d %d %d 0x%x %d %d %d %d\n" % (idx, v1, v2, flags, special, tag, sidenum0, sidenum1))


def _dumpSIDEDEFS(destdir, raw):
    path = os.path.join(destdir, "SIDEDEFS%stxt" % os.path.extsep)
    with open(path, "wt") as fp:
        fp.write("idx x_shift y_shift uppertexture lowertexture middletexture sector\n")
        for idx, sraw in enumerate(wad.chopBytes(raw, 30)):
            xoff, yoff, toptex, bottomtex, middletex, sector = struct.unpack("<hh8s8s8sh", sraw)
            fp.write("%d %d %d %s %s %s %d\n" % (idx, xoff, yoff, wad.pythonifyString(toptex), wad.pythonifyString(bottomtex), wad.pythonifyString(middletex), sector))


def _dumpSECTORS(destdir, raw):
    path = os.path.join(destdir, "SECTORS%stxt" % os.path.extsep)
    with open(path, "wt") as fp:
        fp.write("idx floor ceiling floortexture ceilingtexture light special tag\n")
        for idx, sraw in enumerate(wad.chopBytes(raw, 26)):
            f_height, c_height, f_tex, c_tex, light, special, tag = struct.unpack("<hh8s8shhh", sraw)
            fp.write("%d %d %d %s %s %d %d %d\n" % (idx, f_height, c_height, wad.pythonifyString(f_tex), wad.pythonifyString(c_tex), light, special, tag))


def _dumpMap(w, mapname, lumps):
    if not os.path.isdir(mapname):
        if os.path.isfile(mapname):
            raise ValueError("map file \"%s\" already exists" % mapname)
        os.mkdir(mapname)

    dumpout = {}
    dumpout["THINGS"] = _dumpTHINGS
    dumpout["VERTEXES"] = _dumpVERTEXES
    dumpout["LINEDEFS"] = _dumpLINEDEFS
    dumpout["SIDEDEFS"] = _dumpSIDEDEFS
    dumpout["SECTORS"] = _dumpSECTORS

    for l in lumps:
        if l.name in dumpout:
            dumpout[l.name](mapname, w.readLump(l))


def main(argv):
    if len(argv) < 3:
        print "usage: %s <wad> mapname" % argv[0]
        sys.exit(0)

    w = wad.Wad(argv[1])

    for mapname in argv[2:]:
        mapname = mapname.upper()
        _dumpMap(w, mapname, _findMapLumps(w, mapname))

    w.close()


if __name__ == "__main__":
    main(sys.argv)
