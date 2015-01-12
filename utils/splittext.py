"""
Another simple token parser. Generator yielding tokens out of a
multi-line string.

Tokens are white-space, quote, or comment dilimited. Quoted tokens
with escaped characters are supported.

Example:
    ' token1 token2 "quoted token" # comment until end of line '
    ' token1 "quoted with \"escape\" sequences" '
"""

import string


def splitText(text):
    idx = 0
    while idx < len(text):
        while idx < len(text) and text[idx] in string.whitespace:
            idx += 1

        if idx == len(text):
            # end of input
            break
        elif text[idx] == "#":
            # comment; skip to next line
            while idx < len(text) and text[idx] not in "\r\n":
                idx += 1
        elif text[idx] == "\"":
            # quoted token
            tok, idx = _parseQuotedToken(text, idx)
            yield tok
        else:
            # normal token
            tok, idx = _parseNormalToken(text, idx)
            yield tok


def _parseNormalToken(text, start):
    idx = start
    while idx < len(text) and text[idx] not in string.whitespace + "#\"":
        idx += 1
    return (text[start:idx], idx)


_escape_xlate = { "0": "\x00",
                  "a": "\a",
                  "b": "\b",
                  "t": "\t",
                  "n": "\n",
                  "v": "\v",
                  "f": "\f",
                  "r": "\r",
                  "e": "\x1b" }

def _parseQuotedToken(text, start):
    idx = start + 1 # skip initial quote

    ret = ""
    while 1:
        start = idx
        while idx < len(text) and text[idx] not in "\\\"":
            idx += 1

        ret += text[start:idx]

        if idx == len(text):
            # no terminating quote
            raise ValueError("unterminated quoted token")
        elif text[idx] == "\"":
            # end of quoted token
            idx += 1
            break
        else:
            # escape character
            idx += 1
            if idx == len(text):
                raise ValueError("escape sequence terminated by eof")

            if text[idx] in _escape_xlate:
                ret += _escape_xlate[text[idx]]
            else:
                ret += text[idx]

            idx += 1

    return (ret, idx)
