#!/usr/bin/env python3

import sys
import collections

import wadmap
import wad
import vec


Choice = collections.namedtuple("Choice", "orig imbalance crosscount")

Node = collections.namedtuple("Node", "idx v1 v2 front back")


class Line(vec.Line2D):
    def __init__(self, l, orig):
        super().__init__(l)
        self.orig = orig

    def splitLine(self, other, include_on=True):
        # always include on
        side = self.lineSide(other, include_on=True)

        front = None
        back = None
        on = None

        if side == vec.SIDE_ON:
            on = other
        elif side == vec.SIDE_FRONT:
            front = other
        elif side == vec.SIDE_BACK:
            back = other
        elif side == vec.SIDE_CROSS:
            front, back = self._splitCrossingLine(other)
            # fragments should preserve the orig reference
            if front:
                front = Line(front, other.orig)
            if back:
                back = Line(back, other.orig)
        else:
            raise Exception("unknown side {}".format(side))

        return (front, back, on)


IMBALANCE_CUTOFF = 0.5


def _chooseNode(choices):
    axial = filter(lambda c: c.orig.axial, choices)
    nonaxial = filter(lambda c: not c.orig.axial, choices)

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


def recursiveBSP(lines, outnodes):
    # first, find possible cutting nodes
    choices = []
    for l in lines:
        front, back, cross, on = l.orig.countLinesSides(lines)
        if cross or (front and back):
            c = Choice(l.orig,
                       abs(front - back) / float(len(lines)),
                       cross)
            choices.append(c)

    if not choices:
        # a leaf
        return None

    # a splitting node

    # reserve its spot in the node list before recursing
    idx = len(outnodes)
    outnodes.append(None)

    choice = _chooseNode(choices)
    front, back, on = choice.orig.splitLines(lines)
    f = recursiveBSP(front, outnodes)
    b = recursiveBSP(back, outnodes)
    # lines on the node no longer contribute to nodes calc

    outnodes[idx] = Node(idx,
                         (choice.orig[0][0], choice.orig[0][1]),
                         (choice.orig[1][0], choice.orig[1][1]),
                         f,
                         b)

    return outnodes[idx]


def makeNodes(w, mapname):
    vertexes = list(wadmap.iterVertexes(wadmap.lumpForMap(w, mapname, "VERTEXES")))
    linedefs = list(wadmap.iterLinedefs(wadmap.lumpForMap(w, mapname, "LINEDEFS")))

    # Note we negate y-axis coordinates from the WAD to match our
    # coordinate system. Consequently, we must reverse the linedef
    # vertex ordering to keep normals correct.
    origs = [ vec.Line2D((vertexes[ld.v2].x, -vertexes[ld.v2].y), \
                         (vertexes[ld.v1].x, -vertexes[ld.v1].y)) \
              for ld in linedefs ]
    lines = [Line(o, o) for o in origs]
    for l in lines:
        # each orig line is its own orig
        l.orig = l

    nodes = []
    recursiveBSP(lines, nodes)
    return nodes


def _num(val):
    ival = int(val)
    if val == ival:
        return str(ival)
    return str(val)


def printNodes(nodes):
    print("# idx x1 y1 x2 y2 front_idx back_idx")
    for n in nodes:
        s = "{}".format(n.idx)
        s += " {} {}".format(_num(n.v1[0]), _num(n.v1[1]))
        s += " {} {}".format(_num(n.v2[0]), _num(n.v2[1]))
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
    if len(argv) != 3:
        print("Calculate the node lines for a map")
        print("usage: {} <wad> <map>".format(argv[0]))
        sys.exit(0)

    w = wad.Wad(argv[1])
    mapname = argv[2]
    nodes = makeNodes(w, mapname)
    printNodes(nodes)


if __name__ == "__main__":
    main(sys.argv)
