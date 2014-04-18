#!/usr/bin/env python

"""
"""

import sys
import os
import struct

from PyQt4 import QtCore
from PyQt4 import QtGui

class Level(object):
    width = 0
    height = 0
    pixels = None

class Texture(object):
    name = ""
    original_width = 0
    original_height = 0
    masked = False
    levels = None

TEXTURE_VERSION = 1

app = None
win = None
last_dir = ""

#images = None
texture = None

class TexWidget(QtGui.QLabel):
    def heightForWidth(self, w):
        return self.pixmap().height() * (w / float(self.pixmap().width()))


def refreshGUI():
    vbox = win.centralWidget().layout()
    while vbox.count() > 0:
        item = vbox.takeAt(0)
        w = item.widget()
        w.setParent(None)
        w.deleteLater()

    if not texture or not texture.levels:
        return

    for level in texture.levels:
        if texture.masked:
            fmt = QtGui.QImage.Format_ARGB32
        else:
            fmt = QtGui.QImage.Format_RGB888
        image = QtGui.QImage(level.width, level.height, fmt)
        idx = 0
        for y in xrange(level.height):
            for x in xrange(level.width):
                image.setPixel(x, y, level.pixels[idx])
                idx += 1

        pixmap = QtGui.QPixmap(image.width(), image.height())
        pixmap.convertFromImage(image)

        label = TexWidget()
        label.setBackgroundRole(QtGui.QPalette.Base)
#       label.setSizePolicy(QtGui.QSizePolicy.Fixed, QtGui.QSizePolicy.Ignored)
#       label.setSizePolicy(QtGui.QSizePolicy.Ignored, QtGui.QSizePolicy.Fixed)
#       label.setSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
#       sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Preferred)
#       sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Ignored, QtGui.QSizePolicy.Ignored)
#       sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Expanding)
        sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Preferred, QtGui.QSizePolicy.Expanding)
#       sp = QtGui.QSizePolicy(QtGui.QSizePolicy.Expanding, QtGui.QSizePolicy.Preferred)
        sp.setHeightForWidth(True)
        label.setSizePolicy(sp)
        label.setScaledContents(True)
        label.setPixmap(pixmap)
        if level == texture.levels[0]:
            ttip = "%dx%d (original %dx%d)" % (level.width, level.height, texture.original_width, texture.original_height)
        else:
            ttip = "%dx%d" % (level.width, level.height)
        label.setToolTip(ttip)

#       hbox = QtGui.QHBoxLayout()
#       hbox.addWidget(label)
#       hbox.addStretch()
#       vbox.addLayout(hbox)
        vbox.addWidget(label)


def _loadSimpleTexture(fp):
    fp.seek(0, 2)
    fsize = fp.tell()
    fp.seek(0)

    if fsize <= 8:
        raise ValueError("not an image")

    width, = struct.unpack("<I", fp.read(4))
    height, = struct.unpack("<I", fp.read(4))
    if width <= 0 or width > 1024 or height <= 0 or height > 1024:
        raise ValueError("invalid size %dx%d" % (width, height))

    if fsize != 8 + (width * height * 3):
        if fsize != 8 + (width * height * 3) + (width * height):
            raise ValueError("invalid file size %d" % fsize)

    pixeldat = fp.read(width * height * 3)

    if fp.tell() == fsize:
        pixels = [ QtGui.qRgb(ord(pixeldat[i + 0]), \
                              ord(pixeldat[i + 1]), \
                              ord(pixeldat[i + 2])) for i in xrange(0, len(pixeldat), 3) ]
        masked = False
    else:
        maskdat = fp.read(width * height)
        pixels = []
        for idx in xrange(width * height):
            if maskdat[idx] != "\x00":
                rgba = QtGui.qRgba(ord(pixeldat[idx * 3 + 0]), \
                                   ord(pixeldat[idx * 3 + 1]), \
                                   ord(pixeldat[idx * 3 + 2]), \
                                   255)
            else:
                rgba = QtGui.qRgba(0, 0, 0, 0)
            pixels.append(rgba)
        masked = True

    levels = []

    lev = Level()
    lev.width = width
    lev.height = height
    lev.pixels = pixels
    levels.append(lev)

    ret = Texture()
    ret.name = "none"
    ret.original_width = width
    ret.original_height = height
    ret.masked = bool(masked)
    ret.levels = levels

    return ret


def _loadTexture(fp):
    fp.seek(0)

    if fp.read(8) != "TEXMIPS\x00":
        return _loadSimpleTexture(fp)

    version, = struct.unpack("<H", fp.read(2))
    if version != TEXTURE_VERSION:
        raise ValueError("invalid version 0x%x" % version)

    name = fp.read(8)
    if "\x00" in name:
        name = name[:name.index("\x00")]

    width, = struct.unpack("<H", fp.read(2))
    height, = struct.unpack("<H", fp.read(2))
    original_width, = struct.unpack("<H", fp.read(2))
    original_height, = struct.unpack("<H", fp.read(2))
    masked, = struct.unpack("<H", fp.read(2))
    mipdatalen, = struct.unpack("<I", fp.read(4))

    mipdata = fp.read(mipdatalen)

    levels = []

    w = width
    h = height
    idx = 0
    while idx < len(mipdata):
        pixeldat = mipdata[idx:idx + (w * h * 3)]
        idx += w * h * 3

        if bool(masked):
            bytes_needed = ((w * h) + 7) / 8
            maskdat = mipdata[idx:idx + bytes_needed]
            idx += bytes_needed
        else:
            maskdat = None

        if not maskdat:
            pixels = [ QtGui.qRgb(ord(pixeldat[i + 0]), \
                                  ord(pixeldat[i + 1]), \
                                  ord(pixeldat[i + 2])) for i in xrange(0, len(pixeldat), 3) ]
        else:
            pixels = []
            for pixidx in xrange(len(pixeldat) / 3):
                if (ord(maskdat[pixidx >> 3]) & (1 << (pixidx & 0x7))) != 0x0:
                    rgba = QtGui.qRgba(ord(pixeldat[pixidx * 3 + 0]), \
                                       ord(pixeldat[pixidx * 3 + 1]), \
                                       ord(pixeldat[pixidx * 3 + 2]), \
                                       255)
                else:
                    rgba = QtGui.qRgba(ord(pixeldat[pixidx * 3 + 0]), \
                                       ord(pixeldat[pixidx * 3 + 1]), \
                                       ord(pixeldat[pixidx * 3 + 2]), \
                                       0)
                pixels.append(rgba)

        lev = Level()
        lev.width = w
        lev.height = h
        lev.pixels = pixels

        levels.append(lev)

        w /= 2
        h /= 2

    ret = Texture()
    ret.name = name
    ret.original_width = original_width
    ret.original_height = original_height
    ret.masked = bool(masked)
    ret.levels = levels

    return ret


def openDoc(path):
    global last_dir
    global texture

    with open(path, "rb") as fp:
        texture = _loadTexture(fp)

    win.setWindowTitle(path)
    last_dir = os.path.dirname(path)
    refreshGUI()


def _runOpenDoc():
    path = QtGui.QFileDialog.getOpenFileName(parent=win, caption="Open", directory=last_dir)
    path = str(path)
    if not path:
        return

    try:
        openDoc(path)
    except Exception as e:
        QtGui.QMessageBox.warning(win, "Error", str(e))
        return


if __name__ == "__main__":
    app = QtGui.QApplication(sys.argv)

    win = QtGui.QMainWindow()

    m = win.menuBar().addMenu("&File")
    a = m.addAction("&Open", _runOpenDoc)
    a.setShortcut(QtGui.QKeySequence.Open)
    a = m.addAction("E&xit", win.close)
    a.setShortcut(QtGui.QKeySequence.Quit)

    w = QtGui.QWidget()
    w.setLayout(QtGui.QVBoxLayout())
    win.setCentralWidget(w)
    win.setWindowTitle("[No Name]")
    win.setWindowIcon(QtGui.QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"))
    win.show()
    win.resize(848, 720)

    last_dir = os.path.abspath(".")

    if len(sys.argv) > 1:
        openDoc(sys.argv[1])

    sys.exit(app.exec_())




