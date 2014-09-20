#!/usr/bin/env python3

"""
Utility to load/save DOOM engine wad files.
"""

import struct


def iterChopBytes(b, chunk_len, count=None, start_offset=0):
    """
    Run through a string of bytes, yielding consecutive chunks out of
    it. A starting byte offset can be given.
    Note the last chunk could be less than chunk_len if the total length
    isn't a multiple of chunk_len.
    """

    if count is not None:
        idx = start_offset
        while count > 0:
            yield b[idx: idx + chunk_len]
            idx += chunk_len
            count -= 1
    else:
        idx = start_offset
        while idx < len(b):
            yield b[idx: idx + chunk_len]
            idx += chunk_len


def wadBytesToString(b):
    """
    Convert a byte sequence read from a wad file to a python string.
    """

    if b"\x00" in b:
        b = b[:b.index(b"\x00")]
    return b.decode().upper()


def stringToWadBytes(s):
    """
    Format a string to be used as a string stored in a wad file.
    """

    b = s.upper().encode()
    return b + b"\x00" * (8 - len(b))


def writeWad(path, lumps):
    """
    Write lumps out to a wad. The lumps array should be tuples in
    (name, data) form.
    """

    with open(path, "wb") as fp:
        # dummy header, will get overwritten later
        fp.write(b"\x00" * 12)

        # lump data
        offs = []
        for lumpname, lumpdata in lumps:
            offs.append(fp.tell())
            fp.write(lumpdata)

        # entry table
        infotableofs = fp.tell()
        for offset, (lumpname, lumpdata) in zip(offs, lumps):
            fp.write(struct.pack("<i", offset))
            fp.write(struct.pack("<i", len(lumpdata)))
            fp.write(stringToWadBytes(lumpname))

        # header
        fp.seek(0)
        fp.write(b"PWAD")
        fp.write(struct.pack("<i", len(lumps)))
        fp.write(struct.pack("<i", infotableofs))


class WadLump(object):
    """
    On disk, each entry is 16 bytes. 8 for the name, 4 for the filepos,
    4 for the size.
    """

    DISK_SIZE = 16

    def __init__(self):
        self.filepos = 0
        self.size = 0
        self.name = ""

    @classmethod
    def newFromBytes(cls, raw, offs):
        ret = cls()
        ret.filepos, = struct.unpack("<i", raw[offs + 0:offs + 4])
        ret.size, = struct.unpack("<i", raw[offs + 4:offs + 8])
        ret.name = wadBytesToString(raw[offs + 8:offs + 16])
        return ret


class Wad(object):
    """
    Utility to read from wad files.
    """

    def __init__(self, path=""):
        self.lumps = []
        self.lump_name_to_num = {}
        self.lump_names = []

        self._handle = None

        if path:
            self.open(path)

    def open(self, path):
        handle = open(path, "rb")

        if handle.read(4) not in (b"IWAD", b"PWAD"):
            raise Exception("\"%s\" is not a valid wad file" % path)

        numlumps, = struct.unpack("<i", handle.read(4))
        infotableofs, = struct.unpack("<i", handle.read(4))

        handle.seek(infotableofs)
        raw = handle.read(numlumps * WadLump.DISK_SIZE)
        lumps = [WadLump.newFromBytes(raw, idx * WadLump.DISK_SIZE) for idx in range(numlumps)]

        self.close()
        self.lumps = lumps
        self.lump_name_to_num = { l.name: idx for idx, l in enumerate(self.lumps) }
        self.lump_names = [l.name for l in self.lumps]
        self._handle = handle

    def close(self):
        if self._handle:
            self.lumps = []
            self.lump_name_to_num = {}
            self.lump_names = []
            self._handle.close()
            self._handle = None

    def readLumpFromOffset(self, lumpname, offs):
        while offs < len(self.lumps):
            if self.lumps[offs].name == lumpname:
                return self.readLump(self.lumps[offs])
            offs += 1
        raise Exception("unable to find lump \"%s\"" % lumpname)

    def readLump(self, l):
        if isinstance(l, int):
            l = self.lumps[l]
        elif isinstance(l, str):
            l = self.lumps[self.lump_name_to_num[l]]
        elif isinstance(l, WadLump):
            pass
        else:
            raise Exception("invalid lump \"%s\"" % l)

        return self.readBytes(l.filepos, l.size)

    def readBytes(self, offset, count):
        self._handle.seek(offset)
        return self._handle.read(count)


if __name__ == "__main__":
    import sys

    if len(sys.argv) == 1:
        pass
    elif len(sys.argv) == 2:
        w = Wad(sys.argv[1])
        print("offset size name")
        for l in w.lumps:
            print(l.filepos, l.size, l.name)
        print("%d lumps" % len(w.lumps))
    else:
        w = Wad(sys.argv[1])
        for lumpname in sys.argv[2:]:
            lumpname = lumpname.upper()
            dat = w.readLump(lumpname)
            fp = open(lumpname, "wb")
            fp.write(dat)
            fp.close()
            print("wrote %d bytes to \"%s\"" % (len(dat), lumpname))
