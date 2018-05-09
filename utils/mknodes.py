#!/usr/bin/env python3

import sys

import wadmap
import wad
import vec

#FIXME: output node's original line; not chopped line

class Choice(object):
    line = None
    imbalance = 0.0
    crosscount = 0

class Node(object):
    idx = -1
    line = None
    front = None
    back = None

_nodes = []


IMBALANCE_CUTOFF = 0.5

def _chooseNode(choices):
    axial = filter(lambda c: c.line.axial, choices)
    nonaxial = filter(lambda c: not c.line.axial, choices)

    for by_axis_list in (axial, nonaxial):
        best = None
        for c in sorted(by_axis_list, key=lambda x: x.imbalance):
            if c.imbalance > IMBALANCE_CUTOFF and best is not None:
                # don't choose a node with wildly imbalanced children
                break
            if best is None:
                best = c
            elif c.crosscount < best.crosscount:
                best = c
        if best:
            return best

    # shouldn't happen
    raise Exception("no node chosen")


def recursiveBSP(lines):
    # first, find possible cutting nodes
    choices = []
    for l in lines:
        front, back, cross, on = l.countLinesSides(lines)
        if cross or (front and back):
            c = Choice()
            c.line = l
            c.imbalance = abs(front - back) / float(len(lines))
            c.crosscount = cross
            choices.append(c)

    if not choices:
        # a leaf
        return None

    # a splitting node

    choice = _chooseNode(choices)

    front, back, on = choice.line.splitLines(lines)

    n = Node()
    n.idx = len(_nodes)
    _nodes.append(n)
    n.line = choice.line
    n.front = recursiveBSP(front)
    n.back = recursiveBSP(back)

    return n


def _num(val):
    ival = int(val)
    if val == ival:
        return str(ival)
    return str(val)


def makeNodes(w, mapname):
    global _nodes

    vertexes = list(wadmap.iterVertexes(wadmap.lumpForMap(w, mapname, "VERTEXES")))
    linedefs = list(wadmap.iterLinedefs(wadmap.lumpForMap(w, mapname, "LINEDEFS")))

    # Note we negate y-axis coordinates from the WAD to match our
    # coordinate system. Consequently, we must reverse the linedef
    # vertex ordering to keep normals correct.
    lines = [ vec.Line2D((vertexes[ld.v2].x, -vertexes[ld.v2].y), \
                         (vertexes[ld.v1].x, -vertexes[ld.v1].y)) \
              for ld in linedefs ]

    _nodes = []
    recursiveBSP(lines)

    print("# idx x1 y1 x2 y2 front_idx back_idx")
    for idx, n in enumerate(_nodes):
        s = "{}".format(idx)
        s += " {} {}".format(_num(n.line[0][0]), _num(n.line[0][1]))
        s += " {} {}".format(_num(n.line[1][0]), _num(n.line[1][1]))
        if n.front is not None:
            s += " {}".format(n.front.idx)
        else:
            s += " \"leaf\""
        if n.back is not None:
            s += " {}".format(n.back.idx)
        else:
            s += " \"leaf\""
        print(s)


def main(argv):
    if len(argv) < 3:
        print("Calculate the node lines for a map")
        print("usage: {} <wad> <map> ...".format(argv[0]))
        sys.exit(0)

    w = wad.Wad(argv[1])
    for mapname in argv[2:]:
        makeNodes(w, mapname)


if __name__ == "__main__":
    main(sys.argv)
