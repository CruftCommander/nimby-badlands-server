#!/usr/bin/env python3
# read_mlod.py - independent MLOD P3D reader used to check what build_nimbydpv_model.py
# wrote: header, each LOD's counts, resolution, taggs, selection sizes, bounding box, and
# the proxy triangles' decoded origins. Pure Python, no Blender.
#   python3 read_mlod.py <file.p3d>
import struct, sys, math

def asciiz(buf, pos):
    end = buf.index(b"\0", pos)
    return buf[pos:end].decode("ascii", "replace"), end + 1

def read(path):
    b = open(path, "rb").read()
    assert b[:4] == b"MLOD", "not MLOD"
    version, nlods = struct.unpack_from("<II", b, 4)
    pos = 12
    print("%s: MLOD version 0x%x, %d LODs, %d bytes" % (path, version, nlods, len(b)))
    for li in range(nlods):
        sig = b[pos:pos + 4]; assert sig == b"P3DM", sig
        major, minor, npts, nnorm, nfaces, flags = struct.unpack_from("<IIIIII", b, pos + 4)
        pos += 28
        pts = []
        for i in range(npts):
            x, y, z, fl = struct.unpack_from("<fffI", b, pos); pos += 16
            pts.append((x, y, z))
        pos += 12 * nnorm
        faces = []
        for i in range(nfaces):
            ftype = struct.unpack_from("<I", b, pos)[0]; pos += 4
            corners = []
            for k in range(4):
                pi, ni, u, v = struct.unpack_from("<IIff", b, pos); pos += 16
                if k < ftype:
                    corners.append((pi, ni, u, v))
            fflags = struct.unpack_from("<I", b, pos)[0]; pos += 4
            tex, pos = asciiz(b, pos); mat, pos = asciiz(b, pos)
            faces.append((corners, tex, mat))
        assert b[pos:pos + 4] == b"TAGG", b[pos:pos + 4]; pos += 4
        taggs = []
        while True:
            active = b[pos]; pos += 1
            name, pos = asciiz(b, pos)
            n = struct.unpack_from("<I", b, pos)[0]; pos += 4
            data = b[pos:pos + n]; pos += n
            taggs.append((name, data))
            if name == "#EndOfFile#":
                break
        res = struct.unpack_from("<f", b, pos)[0]; pos += 4
        if pts:
            xs, ys, zs = zip(*pts)
            bb = "x %.2f..%.2f  y %.2f..%.2f  z %.2f..%.2f" % (min(xs), max(xs), min(ys), max(ys), min(zs), max(zs))
        else:
            bb = "-"
        texs = sorted({t for _, t, _ in faces if t})
        print("LOD %d res=%g points=%d normals=%d faces=%d  bbox %s" % (li, res, npts, nnorm, nfaces, bb))
        if texs:
            print("   textures: %s" % ", ".join(texs))
        for name, data in taggs:
            if name.startswith("#"):
                if name == "#Property#":
                    k, _ = asciiz(data, 0); v, _ = asciiz(data, 64)
                    print("   property %s=%s" % (k, v))
                elif name == "#Mass#":
                    m = struct.unpack("<%df" % (len(data) // 4), data)
                    print("   mass total %.1f kg over %d points" % (sum(m), len(m)))
                elif name != "#EndOfFile#":
                    print("   %s (%d bytes)" % (name, len(data)))
                continue
            assert len(data) == npts + nfaces, (name, len(data), npts, nfaces)
            sp = sum(1 for x in data[:npts] if x); sf = sum(1 for x in data[npts:] if x)
            extra = ""
            if name.startswith("proxy:") and sf:
                fi = next(i for i, x in enumerate(data[npts:]) if x)
                corners = faces[fi][0]
                p = [pts[c[0]] for c in corners]
                # origin = vertex at the largest internal angle
                def ang(i):
                    a, bq, c = p[i], p[(i + 1) % 3], p[(i + 2) % 3]
                    u = (bq[0]-a[0], bq[1]-a[1], bq[2]-a[2]); w = (c[0]-a[0], c[1]-a[1], c[2]-a[2])
                    du = math.sqrt(sum(t*t for t in u)); dw = math.sqrt(sum(t*t for t in w))
                    return math.acos(max(-1, min(1, sum(x*y for x, y in zip(u, w)) / (du*dw))))
                o = p[max(range(3), key=ang)]
                extra = "  origin=(%.2f, %.2f, %.2f)" % o
            elif nfaces == 0 and sp <= 2:
                extra = "  " + " ".join("(%.2f, %.2f, %.2f)" % pts[i] for i, x in enumerate(data[:npts]) if x)
            print("   sel %-45s points=%-5d faces=%d%s" % (name, sp, sf, extra))
    assert pos == len(b), "trailing bytes: %d" % (len(b) - pos)
    print("OK: file fully consumed")

if __name__ == "__main__":
    read(sys.argv[1])
