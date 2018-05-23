import math


def _xyzBin(p, binsize):
    def _bin(coord):
        i = math.floor(coord)
        return (i - (i % binsize)) // binsize
    return (_bin(p[0]), _bin(p[1]), _bin(p[2]))


def _iterSurroundingBins(b):
    for x in (b[0] - 1, b[0], b[0] + 1):
        for y in (b[1] - 1, b[1], b[1] + 1):
            for z in (b[2] - 1, b[2], b[2] + 1):
                yield (x, y, z)


class VertexPool():
    DIST_CUTOFF = 0.01
    BIN_CUBE_SIZE = 50

    def __init__(self):
        self.verts = []

        # each bin is a list of indices into verts
        self._bins = {}

    def _checkdist(self, a, b):
        # quick axial check first
        for i in range(3):
            if math.fabs(a[i] - b[i]) > 0.5:
                return False

        # pretty close to each other; do full distance check

        d = math.sqrt( \
                ((a[0] - b[0]) ** 2) + \
                ((a[1] - b[1]) ** 2) + \
                ((a[2] - b[2]) ** 2))
        return d < self.DIST_CUTOFF

    def add(self, v):
        """
            Returns the index of the vert
        """

        b = _xyzBin(v, self.BIN_CUBE_SIZE)

        # first check surrounding bins if there's already a
        # nearly-identical vert

        for k in _iterSurroundingBins(b):
            if k in self._bins:
                for vidx in self._bins[k]:
                    if self._checkdist(v, self.verts[vidx]):
                        return (vidx, self.verts[vidx])

        # new vertex

        if b not in self._bins:
            self._bins[b] = []
        vidx = len(self.verts)
        self.verts.append(v)
        self._bins[b].append(vidx)
        return (vidx, self.verts[vidx])


class EdgePool():
    def __init__(self):
        # edge idx 0 is never used
        self.edges = [None]

    def add(self, v1idx, v2idx):
        try:
            # try forward first
            return self.edges.index((v1idx, v2idx))
        except ValueError:
            try:
                # then backward
                return -self.edges.index((v2idx, v1idx))
            except ValueError:
                # new edge
                idx = len(self.edges)
                self.edges.append((v1idx, v2idx))
                return idx


#class PlanePool():
#    BIN_SIZE = 64
#
#    NORMAL_CUTOFF = 0.00001
#    DIST_CUTOFF = 0.01
#
#    def __init__(self):
#        self.planes = []
#        self._bins = {}
#
#    def add(self, normal, dist):
#        origin = (normal[0] * dist,
#                  normal[1] * dist,
#                  normal[2] * dist)
#
#        # iter through bins surrounding origin
#
#        #...
#
#        return (idx, oppositeside)







#        def _binidx(coord):
#            i = math.floor(coord)
#            return (i - (i % self.BIN_SIZE)) // self.BIN_SIZE
#
#        binxidx = _binidx(v[0])
#        binyidx = _binidx(v[1])
#        binzidx = _binidx(v[2])
#
#        for xi in range(binxidx - 1, binxidx + 2):
#            for yi in range(binyidx - 1, binyidx + 2):
#                for zi in range(binzidx - 1, binzidx + 2):
#                    k = (xi, yi, zi)
#                    if k in self._bins:
#                        for vidx in self._bins[k]:
#                            if self._checkdist(v, self.verts[vidx]):
#                                return vidx
#        for k in _iterBinsSurroundingPoint(v, self.BIN_SIZE):
#
#
#def _iterBinsSurroundingPoint(p, binsize):
#    def _binidx(coord):
#        i = math.floor(coord)
#        return (i - (i % binsize)) // binsize
#
#    binxidx = _binidx(p[0])
#    binyidx = _binidx(p[1])
#    binzidx = _binidx(p[2])
#
#    for xi in range(binxidx - 1, binxidx + 2):
#        for yi in range(binyidx - 1, binyidx + 2):
#            for zi in range(binzidx - 1, binzidx + 2):
#                yield (xi, yi, zi)


