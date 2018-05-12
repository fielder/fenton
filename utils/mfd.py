#!/usr/bin/env python3

# Map From Doom

import collections
import sys

import wadmap
import wad
import vec
import mknodes

Leaf2D = collections.namedtuple("Leaf2D", "lines poly")
Node2D = collections.namedtuple("Node2D", "line onlines front back")

sidedefs = None
sectors = None
innodes = None

InNode = collections.namedtuple("InNode", "line frontidx backidx")


def recurse2D(nodeidx, lines, poly):
    if nodeidx is None:
        # leaf
        for l in lines:
            poly, _ = poly.splitWithLine(l)
        return Leaf2D(lines, poly)
    else:
        # node
        node = innodes[nodeidx]

        flines, blines, olines = node.line.splitLines(lines)

        fpoly, bpoly = poly.splitWithLine(node.line)

        front = recurse2D(node.frontidx, flines, fpoly)
        back = recurse2D(node.backidx, blines, bpoly)

        return Node2D(node.line, olines, front, back)

    raise Exception("shouldn't happen")


def process2D(w, mapname):
    global sidedefs
    global sectors
    global innodes

    nodes = mknodes.makeNodes(w, mapname)
    if not nodes:
        # just a leaf map
        innodes = []
        headnodeidx = None
    else:
        # massage input nodes for processing
        innodes = []
        for n in nodes:
            line = vec.Line2D(n.v1, n.v2)
            if n.front is None:
                frontidx = None
            else:
                frontidx = n.front.idx
            if n.back is None:
                backidx = None
            else:
                backidx = n.back.idx
            innodes.append(InNode(line, frontidx, backidx))
        headnodeidx = 0

    vertexes = list(wadmap.iterVertexes(wadmap.lumpForMap(w, mapname, "VERTEXES")))
    linedefs = list(wadmap.iterLinedefs(wadmap.lumpForMap(w, mapname, "LINEDEFS")))
    sidedefs = list(wadmap.iterSidedefs(wadmap.lumpForMap(w, mapname, "SIDEDEFS")))
    sectors = list(wadmap.iterSectors(wadmap.lumpForMap(w, mapname, "SECTORS")))

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

    process3D(recurse2D(headnodeidx, lines, poly))


def process3D(head):
    print(head)
    pass


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
