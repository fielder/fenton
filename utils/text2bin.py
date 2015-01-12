"""
Convert text values to a binary encoding using struct.pack() formatting.

eg:
  text2bin.binaryFromText( "format <iI 2 98 -1 333" )
gives a binary string containing:
  02 00 00 00 62 00 00 00 ff ff ff ff 4d 01 00 00
"""


import struct
import string

import splittext


def _splitFormat(fmt):
    """
    Split a struct packing format string into its individual components.
    The return includes the total number of items needed to pack with
    the format, including the number of items needed to pack.
    eg:
        "<iI3f8s" -> [ (1, "i"), (1, "I"), (3, "f"), (8, "s") ]
                     6 items needed to pack
    Each tuple is a count and the type of object in the format
    """

    ret = []
    idx = 0

    if idx < len(fmt) and fmt[0] in "@=<>!":
        idx += 1

    while idx < len(fmt):
        if fmt[idx] in "bBhHiIlLqQfd":
            # a single integer or float value
            ret.append( (1, fmt[idx]) )
            idx += 1
        elif fmt[idx] in string.digits:
            # item with a count given
            start = idx
            while idx < len(fmt) and fmt[idx] in string.digits:
                idx += 1
            count = int(fmt[start:idx])
            if idx == len(fmt):
                raise ValueError("count with no format")
            if fmt[idx] not in "bBhHiIlLqQfds":
                # integer, float, or string value
                raise ValueError("invalid counted format \"{}\"".format(fmt[start:idx + 1]))
            item = fmt[idx]
            idx += 1
            ret.append( (count, item) )
        elif fmt[idx] == "s":
            # string items must be given a size
            raise ValueError("string format without size")
        else:
            raise ValueError("invalid format character \"{}\"".format(fmt[idx]))

    # count the total number of items needed to make a packed structure
    # with this format
    num_items_per_pack = 0
    for cnt, f in ret:
        if f == "s":
            # the count for a string object isn't a repeat, but just the
            # length of the string
            num_items_per_pack += 1
        else:
            num_items_per_pack += cnt

    return (num_items_per_pack, tuple(ret))


def _castItems(fmts, items):
    """
    Cast a list of string items to the correct type to be used when
    packing a struct.
    eg:
        fmts: [ (1, "h"), (4, "s"), (2, "f") ]
        items: [ "44", "abcdef", "0.125", "99.5" ]
    gives
        ( 44, b"abcd", 0.125, 99.5 )

    The count given with each format tells how many of that type of item
    must be given.

    Note that the count given with strings is not a repeat. It tells the
    number of characters the string will be packed into. However, for
    this method it's ignored.
    Additionally, because strings are packed, we convert them to byte
    strings here.
    """

    ret = []
    item_idx = 0

    for cnt, fmt in fmts:
        if fmt == "s":
            # String formats are special in that the count isn't a
            # repeat number but number of chars in the string.
            # So for the purposes of casting here the count is ignored.
            # We simply just convert the input value to a byte string.
            if item_idx >= len(items):
                raise ValueError("not enough items given")
            ret.append(items[item_idx].encode())
            item_idx += 1
            continue

        if fmt in "bBhHiIlLqQ":
            while cnt > 0:
                if item_idx >= len(items):
                    raise ValueError("not enough integer items given")
                ret.append(int(items[item_idx], 0))
                item_idx += 1
                cnt -= 1
        elif fmt in "fd":
            while cnt > 0:
                if item_idx >= len(items):
                    raise ValueError("not enough floating-point items given")
                ret.append(float(items[item_idx]))
                item_idx += 1
                cnt -= 1
        else:
            raise ValueError("unknown format \"{}\"".format(fmt))

    return tuple(ret)


def iterBinaryFromText(text):
    """
    Use binaryFromText(), but yield byte strings for every full format
    read.

    eg:
    format <hi 373 449 -1 -9
    Will yield 2 byte strings, each 6 bytes long
    """

    active_format = None
    items_in_format = None
    num_items_per_entry = 0
    token_iter = splittext.splitText(text)
    while 1:
        if active_format is None:
            # start of input; should always begin with a format definition

            try:
                if next(token_iter).lower() != "format":
                    raise ValueError("input should start with a format")
            except StopIteration:
                # empty input? empty output
                break

            try:
                active_format = next(token_iter)
                num_items_per_entry, items_in_format = _splitFormat(active_format)
            except StopIteration:
                raise ValueError("format not given")

            continue

        # process either an entry or a new format definition

        try:
            tok = next(token_iter)
        except StopIteration:
            # end of input
            break

        if tok.lower() == "format":
            # new format given in the middle of the input
            try:
                active_format = next(token_iter)
                num_items_per_entry, items_in_format = _splitFormat(active_format)
            except StopIteration:
                raise ValueError("format not given")

            continue

        # start of a new entry

        try:
            items = [tok]
            while len(items) < num_items_per_entry:
                items.append(next(token_iter))
        except StopIteration:
            raise ValueError("entry missing item(s)")

        casted_items = _castItems(items_in_format, items)
        yield struct.pack(active_format, *casted_items)


def binaryFromText(text):
    """
    Convert a human-readable text description of binary data to actual
    binary data.
    The input text is written as a format specifier string followed by
    values. This format is the same that is used by struct.pack()

    eg:
    format <if8sH
    -1
    8.125
    abcdefgh
    65535

    yields a binary string of bytes:
    ff ff ff ff 00 00 02 41 61 62 63 64 65 66 67 68 ff ff
    ----int---- ---float--- --------string--------- -short-

    Standard struct.pack() endian prefixes are honored.

    Formats can change in the middle of the input:
    format <i 55
    format <f 8.5
    yields a byte string: 37 00 00 00 00 00 08 41

    Multiple values can follow a single format:
    format <h 7 1024 0 -9 32767
    yields a 10-byte binary string

    String formats must be given an output length:
    format 8s blahblah

    A string value longer than it's output length will be truncated.
    A string value shorter than it's output length will be padded with 0
    bytes. This string behavior is provided by struct.pack()

    Note that struct.pack() will not do C-style type casting. For
    example, giving -1 to an unsigned format will not give you 0xffff.
    It will raise a struct.error exception.
    """

    return b"".join(iterBinaryFromText(text))


if False:

    if __name__ == "__main__":
        import sys
        import os
        if len(argv) < 2:
            print("usage: %s infile" % argv[0])
            sys.exit(0)
        for path in argv[1:]:
            # blah.xyz -> blah.xyz.binary
            # But text files are a little special:
            #   blah.txt -> blah.binary
            base, ext = os.path.splitext(path)
            if ext != "" and ext.lower() == ".txt":
                out_path = base + os.path.extsep + "binary"
            else:
                out_path = path + os.path.extsep + "binary"

            text = open(path, "rt").read()
            out_binary = binaryFromText(text)
            with open(out_path, "wb") as fp:
                fp.write(out_binary)
                print("wrote \"{}\"".format(out_path))

else:

    if __name__ == "__main__":
        #TODO: unit tests man!
        pass
