#!/usr/bin/env python3

# Map From Doom

import collections
import sys

import wadmap
import wad
import vec
import mknodes

sidedefs = None
sectors = None
nodes = None


leaf2d = collections.namedtuple("leaf", "lines poly")
node2d = collections.namedtuple("node", "line lines back front")


def recurse2D(nodeidx, lines, poly):
    if nodeidx is None:
        # leaf
        pass
    else:
        # node
        # split lines w/ node
        # split poly w/ node
        node = nodes[nodeidx]
        pass
    return None


def process2D(w, mapname):
    global sidedefs
    global sectors
    global nodes

    nodes = mknodes.makeNodes(w, mapname)
    if not nodes:
        # just a leaf map
        headnode = None
    else:
        headnode = 0

    vertexes = list(wadmap.iterVertexes(wadmap.lumpForMap(w, mapname, "VERTEXES")))
    linedefs = list(wadmap.iterLinedefs(wadmap.lumpForMap(w, mapname, "LINEDEFS")))

    # Note we negate y-axis coordinates from the WAD to match our
    # coordinate system. Consequently, we must reverse the linedef
    # vertex ordering to keep normals correct.
    lines = [ vec.Line2D((vertexes[ld.v2].x, -vertexes[ld.v2].y), \
                         (vertexes[ld.v1].x, -vertexes[ld.v1].y)) \
              for ld in linedefs ]

    minx = min((v.x for v in vertexes)) - 128
    maxx = max((v.x for v in vertexes)) + 128
    miny = min((v.y for v in vertexes)) - 128
    maxy = max((v.y for v in vertexes)) + 128
    poly = vec.Poly2D(((minx, miny),
                       (minx, maxy),
                       (maxx, maxy),
                       (maxx, miny)))

    head = recurse2D(headnode, lines, poly)


def main(argv):
    if len(argv) < 3:
        print("Process a WAD map file to intermediate format")
        print("usage: {} <wad> <map>".format(argv[0]))
        sys.exit(0)

    w = wad.Wad(argv[1])
    mapname = argv[2]

    process2D(w, mapname)
    #...


if __name__ == "__main__":
    main(sys.argv)
