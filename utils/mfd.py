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

InNode = collections.namedtuple("InNode", "line frontidx backidx")
innodes = None

sidedefs = None
sectors = None

#TODO: round off output vertices to 0.001 or so

#FIXME: figure out why we're getting None polys...
def recurse2D(nodeidx, lines, poly):
    if nodeidx is None:
        print("leaf")
#        print(lines)
#        print(poly)
        # leaf
        for l in lines:
            print(type(l))
            poly, _ = poly.splitWithLine(l)
        ret = Leaf2D(lines, poly)
#        print("leaf emit poly: {}".format(poly))
#        print()
        return ret
    else:
        # node
        node = innodes[nodeidx]
        print("node {}".format(nodeidx))

        flines, blines, olines = node.line.splitLines(lines)
        for l in flines:
            print(type(l))
        print()
        for l in blines:
            print(type(l))
        print()
        for l in olines:
            print(type(l))
        print()

        fpoly, bpoly = poly.splitWithLine(node.line)
#        print("f poly {}".format(fpoly))
#        print("b poly {}".format(bpoly))
#        print()

        front = recurse2D(node.frontidx, flines, fpoly)
        back = recurse2D(node.backidx, blines, bpoly)

        ret = Node2D(node.line, olines, front, back)
        return ret

    raise Exception("shouldn't happen")


def process2D(w, mapname):
    global innodes
    global sidedefs
    global sectors

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
#        if True:
#            for n in innodes:
#                print(n)
#            print()

    wadverts = list(wadmap.iterVertexes(wadmap.lumpForMap(w, mapname, "VERTEXES")))
    linedefs = list(wadmap.iterLinedefs(wadmap.lumpForMap(w, mapname, "LINEDEFS")))
    sidedefs = list(wadmap.iterSidedefs(wadmap.lumpForMap(w, mapname, "SIDEDEFS")))
    sectors = list(wadmap.iterSectors(wadmap.lumpForMap(w, mapname, "SECTORS")))

    # Note we negate y-axis coordinates from the WAD to match our
    # coordinate system. Consequently, we must reverse the linedef
    # vertex ordering to keep normals correct.
    ourverts = [(v.x, -v.y) for v in wadverts]
    lines = [ vec.Line2D((ourverts[ld.v2][0], ourverts[ld.v2][1]), \
                         (ourverts[ld.v1][0], ourverts[ld.v1][1])) \
              for ld in linedefs ]

    minx = min((v[0] for v in ourverts)) - 128
    maxx = max((v[0] for v in ourverts)) + 128
    miny = min((v[1] for v in ourverts)) - 128
    maxy = max((v[1] for v in ourverts)) + 128
    poly = vec.Poly2D(((minx, miny),
                       (minx, maxy),
                       (maxx, maxy),
                       (maxx, miny)))
#    print("start poly:")
#    print(poly)
#    print()

    return recurse2D(headnodeidx, lines, poly)


# input should be nodes, leafs
# leaf should have 2d poly + lines w/ linedef refs
# node should have lines w/ linedef refs, front+back node refs
# run through all nodes, leafs:
#  line turned to 3d walls:
#   need to check front+back sidedef, sector
#   a single line could create 0, 1, or 2 3d polys
# create all 3d windings; keep sector ref, texture (high/mid/low)
# merge vertices & create edges, edge loops
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

    bsp2d = process2D(w, mapname)
    print(bsp2d)
    #...


if __name__ == "__main__":
    main(sys.argv)
