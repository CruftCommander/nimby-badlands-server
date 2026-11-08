#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# build_nimbydpv_model.py - DPV model build (Phase 1, M-2).
#
# Reads the carbuni "Desert Patrol Vehicle Low-poly" OBJ (never committed; see
# docs/design/2026-11-05-dpv-model-inventory.md) and writes the two MLOD P3D files
# the NimbyDpv addon needs, plus a JSON report of every decision it made:
#
#   <out>\dpv.p3d        the vehicle: visual LODs 1, 2, 4; Geometry (with #Mass#);
#                        View Geometry; Fire Geometry (damage zones, wheel proxies);
#                        Memory (seat points, axes, lights, fuel, particles);
#                        Hitpoints (no LandContact, as vanilla)
#   <out>\dpvwheel.p3d   the wheel item and proxy model: visual LOD, Geometry, Memory
#   <out>\build-report.json
#
# Runs inside Blender headless (Blender 4.0 to 5.x; developed against 4.0.2, Jeff runs
# 5.2.2 LTS):
#   blender -b -P build_nimbydpv_model.py -- --source <obj> --out <dir>
# Blender is used only for LOD decimation; parsing, classification and the P3D
# writer are plain Python, so `python3 build_nimbydpv_model.py --no-blender ...`
# builds every LOD except the two decimated visual LODs (useful for a quick check).
#
# Coordinates. Source OBJ: centimetres, Z up, nose at -Y, steering wheel at +X.
# Working frame (Blender convention): metres, Z up, nose at +Y, left at -X, ground
# at Z = 0. P3D frame (Real Virtuality): X right, Y up, Z forward, left-handed;
# p3d = (x, z, y) of the working frame, which is the mapping the Arma 3 Object
# Builder add-on uses. UV V is flipped (OBJ origin bottom-left, P3D top-left).
#
# Format references (read November 5, 2026): P3D File Format - MLOD, P3D Lod Faces,
# P3D Named Selections, P3D Model Info (Bohemia community wiki); proxy triangle
# orientation from mrcmodding.gitbook.io "Proxy coordinates" (the vertex at the
# largest internal angle is the origin, the one at the second largest defines +Z,
# the smallest defines +Y).
#
# Vanilla names come from vendor/dayz-1.30-vehicles/offroadhatchback.p3d.names.txt
# and wheeled.config.cpp (recon, "What the DPV model must provide").
# -----------------------------------------------------------------------------
import argparse
import json
import math
import os
import struct
import sys
from collections import Counter, defaultdict

# ----------------------------------------------------------------------------- config
PBO_PREFIX = "NimbyDpv"              # pack.json prefix; paths inside the PBO
DATA = PBO_PREFIX + r"\data"
TEX_BODY = DATA + r"\dpv_body_co.paa"
TEX_WHEEL = DATA + r"\dpv_wheel_co.paa"
TEX_ALPHA = DATA + r"\dpv_alpha_ca.paa"
MAT_BODY = DATA + r"\dpv_body.rvmat"
MAT_WHEEL = DATA + r"\dpv_wheel.rvmat"
MAT_ALPHA = DATA + r"\dpv_alpha.rvmat"
WHEEL_PROXY = "\\" + DATA + r"\dpvwheel"
# inventory slot names (config CfgSlots / attachments[]), in wheel_names order FL, FR, RL, RR
WHEEL_SLOTS = ("nimbydpvwheel_1_1", "nimbydpvwheel_2_1", "nimbydpvwheel_1_2", "nimbydpvwheel_2_2")
SPARE_SLOT = "nimbydpvwheel_spare_1"              # proxy:\NimbyDpv\data\dpvwheel.NNN
# Phase 3 rail storage. History, builds 37 to 46 (the layout below is superseded: since build 47
# all four slots sit on the side shelves, see rail_pos). Builds 37 and 38: four attachment slots
# (config CfgSlots Slot_NimbyDpvRail_N, selection rail_N): 1 and 2 on the side rails beside the
# driver and co-driver for a jerry can, 3 and 4 on the rear cargo deck for an ammo can (Jeff,
# November 7, 09:35). The proxy model is a placeholder: the engine draws whatever item is
# attached in its place, with the item origin at the proxy origin.
# Build 39: the proxy model is what CfgNonAIVehicles keys on (ProxyNimbyDpvRailCan,
# ProxyNimbyDpvRailBox), so the side slots carry the jerry can model and the deck slots the
# ammo can model; an unregistered proxy renders as static decoration (builds 37 and 38).
RAIL_SLOTS = ("nimbydpvrail_1", "nimbydpvrail_2", "nimbydpvrail_3", "nimbydpvrail_4")
# Build 44: the addon's own minimal proxy model for every rail slot (DayZ Modding Knowledge Pack,
# dayz-model-pipeline memory-and-selections.md: "you do NOT reference the vanilla model directly
# in the proxy selection name; create your own proxy .p3d ... the engine then renders the
# attached item's own model at the proxy position"). Written by build_rail_proxy below;
# config class ProxyDpvRail : ProxyAttachment maps it to the four slots.
RAIL_PROXY_MODEL = "\\" + DATA + r"\dpvrail"
RAIL_PROXY = {slot: RAIL_PROXY_MODEL for slot in RAIL_SLOTS}
# Build 53: the backpack twin of each rail slot (config Slot_NimbyDpvRailBag_N, bone railbag_N),
# same proxy model, proxy numbers 5 to 8, its own frame (see railbag_pos below).
RAILBAG_SLOTS = ("nimbydpvrailbag_1", "nimbydpvrailbag_2", "nimbydpvrailbag_3", "nimbydpvrailbag_4")
RAIL_PROXY.update({slot: RAIL_PROXY_MODEL for slot in RAILBAG_SLOTS})
# Build 55: one packed-tent slot per side (config Slot_NimbyDpvRailTent_L/R, bones railtent_l/r),
# proxies 9 and 10, centred on the shelf (see railtent_pos).
RAILTENT_SLOTS = ("nimbydpvrailtent_l", "nimbydpvrailtent_r")
RAIL_PROXY.update({slot: RAIL_PROXY_MODEL for slot in RAILTENT_SLOTS})
CREW_DRIVER_PROXY = r"\dz\vehicles\wheeled\proxies\crew_driver"
CREW_CARGO_PROXY = r"\dz\vehicles\wheeled\proxies\crew_cargo"
PART_PROXIES = {                                      # vanilla part proxies the ADA uses
    "battery": r"\dz\vehicles\parts\battery_car",
    "radiator": r"\dz\vehicles\parts\radiator_car",
    "sparkplug": r"\dz\vehicles\parts\sparkplug",
    "reflector_1_1": r"\dz\vehicles\parts\replacement_headlight",
    "reflector_2_1": r"\dz\vehicles\parts\replacement_headlight",
}

SCALE = 0.01
TOTAL_MASS_KG = 1200.0          # Geometry LOD mass; real DPV 950 kg curb, 1600 loaded
WHEEL_MASS_KG = 15.0
FLIP_WINDING = False            # set True if Object Builder shows the faces inside out

RES_VISUAL = (1.0, 2.0, 4.0)
RES_GEOMETRY = 1.0e13
RES_MEMORY = 1.0e15
RES_LANDCONTACT = 2.0e15
RES_HITPOINTS = 5.0e15
WRITE_HITPOINTS_LOD = False     # see the Hitpoints note in build_model
RES_VIEWGEOMETRY = 6.0e15
RES_FIREGEOMETRY = 7.0e15

# ----------------------------------------------------------------------------- helpers
def v_add(a, b): return (a[0] + b[0], a[1] + b[1], a[2] + b[2])
def v_sub(a, b): return (a[0] - b[0], a[1] - b[1], a[2] - b[2])
def v_scale(a, s): return (a[0] * s, a[1] * s, a[2] * s)
def v_len(a): return math.sqrt(a[0] ** 2 + a[1] ** 2 + a[2] ** 2)
def v_norm(a):
    l = v_len(a)
    return (a[0] / l, a[1] / l, a[2] / l) if l > 1e-12 else (0.0, 0.0, 1.0)
def v_cross(a, b):
    return (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])

def to_p3d(p):
    """Working frame (x right, y forward, z up) to P3D model space. In Bohemia's model space the
    vehicle's FRONT is -Z and its LEFT is +X: the vanilla offroadhatchback.p3d carries its
    headlight and radiator proxies at z = -1.7 and its driver proxy at x = +0.41 (left-hand
    drive), and the Arma2Quad's nose is at -z. The (x, z, y) swap used until November 6, 2026
    put the DPV's front at +Z, so the engine drove it as a car whose front axle was at the back,
    whose driver faced the rear and sat on the wrong side, and whose local-space hub positions
    came out negated in x and z (NIMBY-T-069 diag). This is a 180 degree turn about the up
    axis: x and forward negate, winding is unchanged."""
    return (-p[0], p[2], -p[1])


class Lod:
    """One MLOD LOD under construction, in the working frame."""
    def __init__(self, resolution, name):
        self.resolution = resolution
        self.name = name
        self.points = []            # (x, y, z) working frame
        self.point_flags = []
        self.normals = []           # per face corner, working frame
        self.faces = []             # dict: idx[], nrm[], uv[], tex, mat, flags
        self.selections = defaultdict(lambda: (set(), set()))   # name -> (point set, face set)
        self.properties = []        # (key, value)
        self.mass = None            # list per point for Geometry
        self.sharp_edges = []

    def add_point(self, p, flags=0):
        self.points.append(tuple(p)); self.point_flags.append(flags)
        return len(self.points) - 1

    def add_face(self, idx, nrm, uv, tex="", mat="", flags=0, sel=()):
        base = len(self.normals)
        self.normals.extend(nrm)
        self.faces.append({"idx": list(idx), "nrm": list(range(base, base + len(idx))),
                           "uv": list(uv), "tex": tex, "mat": mat, "flags": flags})
        fi = len(self.faces) - 1
        for s in sel:
            self.selections[s][1].add(fi)
            self.selections[s][0].update(idx)
        return fi

    def select_points(self, name, idxs):
        self.selections[name][0].update(idxs)

    def add_box(self, lo, hi, sel=(), mass=None):
        """Axis-aligned box as 6 quads with outward normals. Returns its point indices."""
        x0, y0, z0 = lo; x1, y1, z1 = hi
        c = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
             (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
        ids = [self.add_point(p) for p in c]
        quads = [((0, 3, 2, 1), (0, 0, -1)), ((4, 5, 6, 7), (0, 0, 1)),
                 ((0, 1, 5, 4), (0, -1, 0)), ((2, 3, 7, 6), (0, 1, 0)),
                 ((1, 2, 6, 5), (1, 0, 0)), ((3, 0, 4, 7), (-1, 0, 0))]
        for q, n in quads:
            self.add_face([ids[i] for i in q], [n] * 4, [(0.0, 0.0)] * 4, sel=sel)
        if mass is not None:
            if self.mass is None:
                self.mass = [0.0] * len(self.points)
            while len(self.mass) < len(self.points):
                self.mass.append(0.0)
            for i in ids:
                self.mass[i] = mass / 8.0
        return ids

    def add_cylinder(self, center, radius, half_width, segments=12, sel=(), mass=None):
        """Cylinder with its axis along working +X (the wheel spin axis), fully triangulated:
        2*segments side triangles plus (segments-2) fan triangles per cap. With 12 segments this is
        the 24-point, 44-face body the Arma2Quad body.p3d uses for every wheel_x_y_damper_land hub
        (read with scripts/dpv/read_mlod.py, November 6, 2026). Returns its point indices."""
        cx, cy, cz = center
        ring = []
        for side in (-1.0, 1.0):
            x = cx + side * half_width
            ids = []
            for i in range(segments):
                a = 2.0 * math.pi * i / segments
                ids.append(self.add_point((x, cy + radius * math.cos(a), cz + radius * math.sin(a))))
            ring.append(ids)
        lo, hi = ring
        for i in range(segments):
            j = (i + 1) % segments
            n0 = v_norm((0.0, self.points[lo[i]][1] - cy, self.points[lo[i]][2] - cz))
            n1 = v_norm((0.0, self.points[lo[j]][1] - cy, self.points[lo[j]][2] - cz))
            # winding follows add_box and the Arma2Quad hubs (P3D clockwise = outward)
            self.add_face([lo[i], hi[j], hi[i]], [n0, n1, n0], [(0.0, 0.0)] * 3, sel=sel)
            self.add_face([lo[i], lo[j], hi[j]], [n0, n1, n1], [(0.0, 0.0)] * 3, sel=sel)
        for ids, n in ((lo, (-1.0, 0.0, 0.0)), (hi, (1.0, 0.0, 0.0))):
            for i in range(1, segments - 1):
                tri = [ids[0], ids[i], ids[i + 1]] if n[0] > 0 else [ids[0], ids[i + 1], ids[i]]
                self.add_face(tri, [n] * 3, [(0.0, 0.0)] * 3, sel=sel)
        allids = lo + hi
        if mass is not None:
            if self.mass is None:
                self.mass = [0.0] * len(self.points)
            while len(self.mass) < len(self.points):
                self.mass.append(0.0)
            for i in allids:
                self.mass[i] = mass / float(len(allids))
        return allids

    def add_proxy(self, path, number, origin, forward=(0, 1, 0), up=(0, 0, 1), sel=(), crew=False):
        """Proxy triangle in Object Builder's convention: v0 at the proxy origin (the right
        angle), v1 = origin + up * 0.001, v2 = origin + forward * 0.002, where forward and up are
        the proxy's local axes in the working frame. The engine reads up from v1 - v0 and forward
        from v2 - v0, which is how the Arma2Quad MLOD wheel proxies (legs 0.001 up, 0.002
        forward) decode to the identity rotation that the vanilla offroadhatchback.p3d ODOL stores
        for its left wheels, and legs with forward negated decode to the 180 degree yaw it stores
        for its right wheels (read November 6, 2026). The triangle used until then (short leg
        0.5 along -forward, long leg 1.0 along up) decoded to a 90 degree pitch: invisible on a
        symmetric wheel, but the hub frame it gave the wheel simulation was never accepted
        (NIMBY-T-069: present=0, every hub at <0,0,0>, with the hub positions proven to come from
        these proxies by the Quad-body hybrids of 10:15)."""
        # "forward" names the P3D +Z leg: P3D +Z is the model's rear, so the Object Builder
        # identity triangle (and the Quad's crew triangle) is produced with forward = (0, -1, 0)
        # in the working frame, and forward = (0, 1, 0) gives the 180 degree yaw
        f = v_norm(forward); u = v_norm(up)
        if crew:
            # crew proxies carry the triangle the Arma2Quad body uses for crew_driver / crew_cargo
            # (right angle at v1: v0 = origin + up * 0.002, v1 = origin, v2 = origin + forward *
            # 0.001), the shape under which its rider sits upright and faces forward on 1.30; the
            # wheel triangle below seated the DPV driver turned 90 degrees (November 6, 2026 11:17)
            p0 = v_add(origin, v_scale(u, 0.002)); p1 = tuple(origin); p2 = v_add(origin, v_scale(f, 0.001))
        else:
            p0 = tuple(origin); p1 = v_add(origin, v_scale(u, 0.001)); p2 = v_add(origin, v_scale(f, 0.002))
        ids = [self.add_point(p0), self.add_point(p1), self.add_point(p2)]
        n = v_norm(v_cross(v_sub(p1, p0), v_sub(p2, p0)))
        name = "proxy:%s.%03d" % (path, number)
        self.add_face(ids, [n] * 3, [(0.0, 0.0)] * 3, sel=tuple(sel) + (name,))
        return ids


# ----------------------------------------------------------------------------- MLOD writer
def encode_weight(w):
    b = int(round(256 - 255 * w))
    return 0 if b >= 256 else b

def write_mlod(path, lods):
    # LODs are written in ascending resolution order, as Object Builder saves them and as every
    # vanilla ODOL and the Arma2Quad MLOD carry them (visual LODs, 1100 shadow, 1e13 geometry,
    # 1e15 memory, 5e15 hitpoints, 6e15 view, 7e15 fire). The append order used until November 6,
    # 2026 (memory and hitpoints after view and fire) survived binarize into the ODOL header, the
    # one structural difference from both references that the selection-name reads could not
    # show (NIMBY-T-069 probe 2f).
    lods = sorted(lods, key=lambda l: l.resolution)
    out = bytearray()
    out += b"MLOD" + struct.pack("<II", 0x101, len(lods))
    for lod in lods:
        n_norm = len(lod.normals)
        out += b"P3DM" + struct.pack("<IIIII", 28, 0x100, len(lod.points), n_norm, len(lod.faces))
        out += struct.pack("<I", 0)
        for p, fl in zip(lod.points, lod.point_flags):
            x, y, z = to_p3d(p)
            out += struct.pack("<fffI", x, y, z, fl)
        for n in lod.normals:
            x, y, z = to_p3d(n)
            out += struct.pack("<fff", x, y, z)
        for f in lod.faces:
            idx, nrm, uv = f["idx"], f["nrm"], f["uv"]
            if FLIP_WINDING:
                idx, nrm, uv = idx[::-1], nrm[::-1], uv[::-1]
            out += struct.pack("<I", len(idx))
            for k in range(4):
                if k < len(idx):
                    out += struct.pack("<IIff", idx[k], nrm[k], uv[k][0], 1.0 - uv[k][1])
                else:
                    out += struct.pack("<IIff", 0, 0, 0.0, 0.0)
            out += struct.pack("<I", f["flags"])
            out += f["tex"].encode("ascii") + b"\0" + f["mat"].encode("ascii") + b"\0"
        out += b"TAGG"
        def tagg(name, data):
            return b"\x01" + name.encode("ascii") + b"\0" + struct.pack("<I", len(data)) + data
        if lod.sharp_edges:
            data = b"".join(struct.pack("<II", a, b) for a, b in lod.sharp_edges)
            out += tagg("#SharpEdges#", data)
        for key, value in lod.properties:
            data = key.encode("ascii").ljust(64, b"\0") + value.encode("ascii").ljust(64, b"\0")
            out += tagg("#Property#", data)
        if lod.mass is not None:
            m = list(lod.mass) + [0.0] * (len(lod.points) - len(lod.mass))
            out += tagg("#Mass#", b"".join(struct.pack("<f", x) for x in m))
        for name, (pts, fcs) in lod.selections.items():
            pb = bytearray(len(lod.points)); fb = bytearray(len(lod.faces))
            for i in pts: pb[i] = encode_weight(1.0)
            for i in fcs: fb[i] = 1
            out += tagg(name, bytes(pb) + bytes(fb))
        out += tagg("#EndOfFile#", b"") + struct.pack("<f", lod.resolution)
    with open(path, "wb") as fh:
        fh.write(out)
    return len(out)


# ----------------------------------------------------------------------------- OBJ parse
def parse_obj(path):
    verts, uvs, faces = [], [], []
    group = "default"
    with open(path, errors="replace") as fh:
        for line in fh:
            t = line.split()
            if not t:
                continue
            if t[0] == "v":
                verts.append((float(t[1]), float(t[2]), float(t[3])))
            elif t[0] == "vt":
                uvs.append((float(t[1]), float(t[2])))
            elif t[0] == "g":
                group = t[1] if len(t) > 1 else "default"
            elif t[0] == "f":
                corners = []
                for tok in t[1:]:
                    parts = tok.split("/")
                    vi = int(parts[0]) - 1
                    ti = int(parts[1]) - 1 if len(parts) > 1 and parts[1] else -1
                    corners.append((vi, ti))
                faces.append((group, corners))
    return verts, uvs, faces


def transform(verts):
    """cm, Z up, nose -Y  ->  m, Z up, nose +Y, left -X, ground at z = 0."""
    out = [(-x * SCALE, -y * SCALE, z * SCALE) for x, y, z in verts]
    zmin = min(p[2] for p in out)
    return [(x, y, z - zmin) for x, y, z in out]


def loose_parts(nverts, faces):
    parent = list(range(nverts))
    def find(a):
        while parent[a] != a:
            parent[a] = parent[parent[a]]; a = parent[a]
        return a
    for _, corners in faces:
        r0 = find(corners[0][0])
        for vi, _ in corners[1:]:
            r = find(vi)
            if r != r0:
                parent[r] = r0
    parts = defaultdict(lambda: {"faces": [], "verts": set()})
    for fi, (group, corners) in enumerate(faces):
        key = (group, find(corners[0][0]))
        parts[key]["faces"].append(fi)
        parts[key]["verts"].update(vi for vi, _ in corners)
    result = []
    for (group, _), d in parts.items():
        result.append({"group": group, "faces": d["faces"], "verts": d["verts"]})
    return result


def bbox(verts, idxs):
    xs = [verts[i][0] for i in idxs]; ys = [verts[i][1] for i in idxs]; zs = [verts[i][2] for i in idxs]
    lo = (min(xs), min(ys), min(zs)); hi = (max(xs), max(ys), max(zs))
    return lo, hi, v_scale(v_add(lo, hi), 0.5), v_sub(hi, lo)


# ----------------------------------------------------------------------------- classify
def classify(verts, parts):
    """Assign every loose part a role. Returns (roles dict part index -> role, facts)."""
    facts = {}
    roles = {}
    wheels = [(i, p) for i, p in enumerate(parts) if p["group"] == "WHEELS" and len(p["verts"]) > 200]
    assert len(wheels) == 4, "expected 4 wheel meshes, found %d" % len(wheels)
    wheel_info = {}
    for i, p in wheels:
        lo, hi, c, size = bbox(verts, p["verts"])
        side = 1 if c[0] < 0 else 2          # 1 = left (-X), 2 = right (+X)
        end = 1 if c[1] > 0 else 2           # 1 = front (+Y), 2 = rear (-Y)
        name = "wheel_%d_%d" % (side, end)
        roles[i] = name
        radius = max(abs(hi[2] - c[2]), abs(hi[1] - c[1]))
        wheel_info[name] = {"center": c, "radius": radius, "width": size[0]}
    facts["wheels"] = wheel_info
    # small WHEELS parts: hub caps (<0.15 m) travel with the wheel item; brake discs stay on the car
    for i, p in enumerate(parts):
        if p["group"] != "WHEELS" or i in roles:
            continue
        lo, hi, c, size = bbox(verts, p["verts"])
        nearest = min(wheel_info, key=lambda n: v_len(v_sub(wheel_info[n]["center"], c)))
        roles[i] = ("hubcap", nearest) if size[1] < 0.15 else ("disc", nearest)
    # BODY and ALPHA
    for i, p in enumerate(parts):
        if i in roles:
            continue
        lo, hi, c, size = bbox(verts, p["verts"])
        nf = len(p["faces"])
        if p["group"] == "BODY" and nf == 144 and abs(c[2] - 1.03) < 0.1 and size[1] < 0.2:
            roles[i] = "drivewheel"; facts["drivewheel"] = {"center": c, "size": size}
        elif p["group"] == "BODY" and nf == 232:
            if abs(c[0]) < 0.1 and c[2] > 1.3:
                roles[i] = "seat_cargo1"; facts["seat_cargo1"] = {"center": c, "lo": lo, "hi": hi}
            elif c[0] < 0:
                roles[i] = "seat_driver"; facts["seat_driver"] = {"center": c, "lo": lo, "hi": hi}
            else:
                roles[i] = "seat_codriver"; facts["seat_codriver"] = {"center": c, "lo": lo, "hi": hi}
        elif p["group"] == "BODY" and c[1] > 1.85 and size[0] > 0.9 and size[2] < 0.3 and size[1] < 0.05:
            roles[i] = "lights"; facts["lights"] = {"center": c, "lo": lo, "hi": hi}
        else:
            roles[i] = "body"
    for key in ("drivewheel", "seat_driver", "seat_codriver", "seat_cargo1", "lights"):
        assert key in facts, "classification missed " + key
    return roles, facts


# ----------------------------------------------------------------------------- visual LOD
def build_visual(verts, uvs, faces, parts, roles, lod, include, split_lights=True):
    """Append the visual mesh of the parts whose role passes include(role) to lod. Keeps
    one P3D point per (vertex) so selections map cleanly. Returns point index map."""
    pmap = {}
    def pt(vi):
        if vi not in pmap:
            pmap[vi] = lod.add_point(verts[vi])
        return pmap[vi]
    lights = roles_fact(parts, roles, "lights")
    for pi, p in enumerate(parts):
        role = roles.get(pi)
        if role is None or not include(role):
            continue
        group = p["group"]
        tex, mat = (TEX_ALPHA, MAT_ALPHA) if group == "ALPHA" else (TEX_WHEEL, MAT_WHEEL) if group == "WHEELS" else (TEX_BODY, MAT_BODY)
        sel = []
        if role == "drivewheel":
            sel.append("drivewheel")
        elif isinstance(role, tuple) and role[0] == "disc":
            # the brake disc moves with the suspension, not the wheel; on the front axle it sits on
            # the steering knuckle (bone wheel_x_1_steering, child of the damper bone), which also
            # gives the turnfrontleft / turnfrontright animations a selection that exists in the
            # visual LODs, as in the Arma2Quad and the vanilla ADA (NIMBY-T-069 probe 2i,
            # November 6, 2026: until then no LOD carried a wheel_x_1_steering selection)
            sel.append(role[1] + "_damper")
        for k, fi in enumerate(p["faces"]):
            _, corners = faces[fi]
            if isinstance(role, tuple) and role[0] == "disc" and role[1].endswith("_1") and k % 2 == 1:
                # the DPV has no knuckle mesh: every other face of the front disc goes to the
                # steering bone instead of the damper bone, so both selections carry real
                # geometry (the two bones are parent and child, so the halves only part by the
                # steering angle, inside the wheel)
                sel = [role[1] + "_steering"]
            elif isinstance(role, tuple) and role[0] == "disc":
                sel = [role[1] + "_damper"]
            idx = [pt(vi) for vi, _ in corners]
            uv = [uvs[ti] if ti >= 0 else (0.0, 0.0) for _, ti in corners]
            # flat face normal from the first three corners
            a, b, c = (verts[corners[0][0]], verts[corners[1][0]], verts[corners[2][0]])
            n = v_norm(v_cross(v_sub(b, a), v_sub(c, a)))
            fsel = list(sel)
            if role == "lights" and split_lights:
                cx = sum(verts[vi][0] for vi, _ in corners) / len(corners)
                fsel.append("light_left" if cx < 0 else "light_right")
            if len(idx) > 4:        # n-gon: fan triangulate
                for k in range(1, len(idx) - 1):
                    lod.add_face([idx[0], idx[k], idx[k + 1]], [n] * 3, [uv[0], uv[k], uv[k + 1]], tex, mat, sel=fsel)
            else:
                lod.add_face(idx, [n] * len(idx), uv, tex, mat, sel=fsel)
    return pmap


def add_antenna(lod, base=(-0.43, -1.45, 2.04), height=0.98, bend=0.12, r0=0.014, r1=0.004, sides=8, segs=10):
    """A whip antenna on the rear-left cage corner (build 47, Jeff, November 7, 2026): a tapered
    tube rising from the cage top (z 2.16 at x -0.45, y -1.45) with a gentle rearward curve, in the
    frame's own texel of the body texture (UV 0.978, 0.617 read from the cage tube beside it).
    Visual only: a static curve reads as flexible; real flex would need a model.cfg animation.
    Build 48: base lowered 8 cm into the tube and widened (it hovered an inch above it on 47)."""
    rings = []
    for k in range(segs + 1):
        t = k / float(segs)
        cx, cy, cz = base[0], base[1] - bend * t * t, base[2] + height * t
        r = r0 + (r1 - r0) * t
        ring = []
        for i in range(sides):
            a = 2.0 * math.pi * i / sides
            ring.append(lod.add_point((cx + r * math.cos(a), cy + r * math.sin(a), cz)))
        rings.append(ring)
    uv = [(0.978, 0.617)] * 4
    for k in range(segs):
        for i in range(sides):
            j = (i + 1) % sides
            a0, a1, b1, b0 = rings[k][i], rings[k][j], rings[k + 1][j], rings[k + 1][i]
            ang = 2.0 * math.pi * (i + 0.5) / sides
            n = (math.cos(ang), math.sin(ang), 0.0)
            lod.add_face([a0, a1, b1, b0], [n] * 4, uv, TEX_BODY, MAT_BODY)
    tip = lod.add_point((base[0], base[1] - bend, base[2] + height + 0.01))
    for i in range(sides):
        j = (i + 1) % sides
        lod.add_face([rings[segs][i], rings[segs][j], tip], [(0.0, 0.0, 1.0)] * 3, uv[:3], TEX_BODY, MAT_BODY)


def add_textured_box(lod, lo, hi, uv_point, sel=()):
    """Axis-aligned box in a visual LOD, every face on one texel of the body texture (the
    antenna's approach), so it needs no UV layout of its own."""
    x0, y0, z0 = lo; x1, y1, z1 = hi
    c = [(x0, y0, z0), (x1, y0, z0), (x1, y1, z0), (x0, y1, z0),
         (x0, y0, z1), (x1, y0, z1), (x1, y1, z1), (x0, y1, z1)]
    ids = [lod.add_point(q) for q in c]
    quads = [((0, 3, 2, 1), (0, 0, -1)), ((4, 5, 6, 7), (0, 0, 1)),
             ((0, 1, 5, 4), (0, -1, 0)), ((2, 3, 7, 6), (0, 1, 0)),
             ((1, 2, 6, 5), (1, 0, 0)), ((3, 0, 4, 7), (-1, 0, 0))]
    for q, n in quads:
        lod.add_face([ids[i] for i in q], [n] * 4, [uv_point] * 4, TEX_BODY, MAT_BODY, sel=sel)
    return ids


def add_wheel_post(lod, spare, hood_z=0.891, post=0.04, top_z=1.15, plate=0.16):
    """Spare-wheel holder on the hood (build 53, Jeff, November 7, 2026: a bare tyre lying on
    the hood "would simply fly off"; wanted something for the rim to be placed over). A square
    post up through the spare's centre on a small base plate, in the frame texel like the
    antenna. Hood under the spare centre (0, 1.45) is z 0.891 in the source mesh, falling
    8.4 degrees toward the nose; the tyre (0.29 wide, centre z 1.06) spans z 0.915 to 1.205, so
    build 53 ran the post to 1.30 (10 cm proud of the tyre); build 54 (Jeff: "a bit too high,
    cut it down by about 6 inches") stops at 1.15, inside the tyre when the spare is on and a
    24 cm stub when it is off. The plate sits 3 cm into the hood so its downhill edge does
    not float. Visual only."""
    uv = (0.978, 0.617)
    cx, cy = spare[0], spare[1]
    h = plate / 2.0
    add_textured_box(lod, (cx - h, cy - h, hood_z - 0.03), (cx + h, cy + h, hood_z + 0.025), uv)
    q = post / 2.0
    add_textured_box(lod, (cx - q, cy - q, hood_z + 0.02), (cx + q, cy + q, top_z), uv)


def roles_fact(parts, roles, name):
    return [i for i, r in roles.items() if r == name]


# ----------------------------------------------------------------------------- decimation (Blender)
def decimate_lod(src_lod, ratio, resolution, name):
    """Rebuild src_lod's visual faces in Blender, decimate, read back. Selections are
    carried as vertex groups; face selections are not kept on the lower LODs."""
    import bpy
    import bmesh
    lod = Lod(resolution, name)
    # one Blender mesh per (texture, material) so the readback keeps the material
    groups = defaultdict(list)
    for f in src_lod.faces:
        if f["tex"]:
            groups[(f["tex"], f["mat"])].append(f)
    sel_names = [n for n in src_lod.selections if not n.startswith("proxy:")]
    for (tex, mat), flist in groups.items():
        bm = bmesh.new()
        uv_layer = bm.loops.layers.uv.new("uv")
        dl = bm.verts.layers.deform.new("w")
        vmap = {}
        used = sorted({i for f in flist for i in f["idx"]})
        for i in used:
            v = bm.verts.new(src_lod.points[i]); vmap[i] = v
        bm.verts.ensure_lookup_table()
        sel_index = {n: k for k, n in enumerate(sel_names)}
        for i in used:
            for n in sel_names:
                if i in src_lod.selections[n][0]:
                    vmap[i][dl][sel_index[n]] = 1.0
        for f in flist:
            try:
                bf = bm.faces.new([vmap[i] for i in f["idx"]])
            except ValueError:
                continue
            for loop, uv in zip(bf.loops, f["uv"]):
                loop[uv_layer].uv = uv
        me = bpy.data.meshes.new("lod")
        bm.to_mesh(me); bm.free()
        ob = bpy.data.objects.new("lod", me)
        for n in sel_names:
            ob.vertex_groups.new(name=n)
        bpy.context.scene.collection.objects.link(ob)
        mod = ob.modifiers.new("dec", "DECIMATE"); mod.ratio = ratio
        dg = bpy.context.evaluated_depsgraph_get()
        ev = ob.evaluated_get(dg).to_mesh()
        uvl = ev.uv_layers.active.data if ev.uv_layers.active else None
        pmap = {}
        gidx = {g.index: g.name for g in ob.vertex_groups}
        for v in ev.vertices:
            pmap[v.index] = lod.add_point(tuple(v.co))
            for g in v.groups:
                if g.weight > 0.5:
                    lod.select_points(gidx[g.group], [pmap[v.index]])
        for poly in ev.polygons:
            idx = [pmap[ev.loops[l].vertex_index] for l in poly.loop_indices]
            uv = [tuple(uvl[l].uv) if uvl else (0.0, 0.0) for l in poly.loop_indices]
            n = tuple(poly.normal)
            if len(idx) > 4:
                for k in range(1, len(idx) - 1):
                    lod.add_face([idx[0], idx[k], idx[k + 1]], [n] * 3, [uv[0], uv[k], uv[k + 1]], tex, mat)
            else:
                lod.add_face(idx, [n] * len(idx), uv, tex, mat)
        ob.evaluated_get(dg).to_mesh_clear()
        bpy.data.objects.remove(ob); bpy.data.meshes.remove(me)
    return lod


# ----------------------------------------------------------------------------- the vehicle
def build_vehicle(verts, uvs, faces, parts, roles, facts, use_blender):
    W = facts["wheels"]
    wheel_names = ["wheel_1_1", "wheel_2_1", "wheel_1_2", "wheel_2_2"]
    half_w = max(abs(verts[i][0]) for p in parts for i in p["verts"])
    front_y = max(verts[i][1] for p in parts for i in p["verts"])
    rear_y = min(verts[i][1] for p in parts for i in p["verts"])
    top_z = max(verts[i][2] for p in parts for i in p["verts"])
    seat_d = facts["seat_driver"]; seat_c = facts["seat_codriver"]; seat_r = facts["seat_cargo1"]
    lights = facts["lights"]
    dw = facts["drivewheel"]

    def crew_origin(seat):
        # the crew proxy origin: bottom of the seat part plus 0.10. The earlier "plus a third of
        # the seat height" (0.84 up) matched the vanilla ADA's crew proxy height, but the DPV's
        # bucket seats sit low under a low cage: the driver's head logged at 1.54 up against a
        # cage top near 1.6 (November 5 and 6, 2026)
        return (seat["center"][0], seat["center"][1], seat["lo"][2] + 0.10)

    # Spare: flat on the hood deck (Jeff, November 6, 2026; builds 1 to 20 had it upright on
    # the rear frame at (0, rear_y + 0.45, 1.35)). The deck is y 1.2 to 1.85 at z 0.85 to
    # 0.92 with the light bar at y 2.0 (source mesh), so a 0.40 m radius, 0.29 m wide tyre
    # centred at y 1.45 lies on the deck and stops short of the lights.
    spare = (0.0, 1.45, 1.06)
    engine = (0.0, rear_y + 0.9, 0.6)
    # Part proxies and their inventory slots (one dict, used by every LOD that carries them).
    # Measured from the source mesh on November 6, 2026: behind y = rear_y + 0.6 the frame
    # narrows to x = +-0.45 (the rear overhang tubes); between the rear wheels (y rear_y + 0.6
    # to rear_y + 1.0) it is x = +-0.8 with the floor at z 0.37. The first placement, both
    # parts at x = +-0.45, y = rear_y + 0.6, z = 0.7, put them on the frame tubes at that
    # transition: the battery hung outside the left rear tube and the radiator, whose origin
    # is its top edge, hung below the rear bumper (build 18 screenshots). Build 19 put the
    # radiator upright across the back of the bay and the battery at (0.58, rear_y + 0.8,
    # 0.45), which is the rear suspension arm, outside the frame again. The radiator is gone
    # since build 20: the DPV is air-cooled (NimbyDpv_Car.IsVitalRadiator). A 0.1 m occupancy
    # map of the source mesh (November 6, 2026) shows the open pocket behind the passenger
    # seat back, under the rear seat and ahead of the engine: x 0.3 to 0.55, y -1.05 to
    # -0.85, z 0.5 to 0.75. The battery sits there, where the real vehicle carries it. The
    # slot boxes and memory points follow these numbers.
    part_pos = {
        # build 46 (Jeff, November 7, 11:18): the battery stands on the rear deck directly behind
        # the rear seat back (y -1.45), between the two seat-back support rails (x +-0.20), base
        # on the deck (z 1.23), origin taken as the model centre; until build 45 it sat in the
        # pocket behind the passenger seat at (0.42, -0.95, 0.62)
        # build 47: flat on the deck (terminals up) behind the rear seat, centred now that the
        # deck carries nothing else; the proxy frame below lays it down
        # build 48 (Jeff, 11:59): down 5 inches (its origin is near its base, not the centre),
        # label face to the rear, and pitched 25 degrees nose-down to sit flush on the deck mesh
        # build 51 (Jeff, build 50: "its bottom flat on the mesh"): the deck top the battery sits
        # on is the ALPHA mesh panel, one plane from z 1.213 at y -1.81 to 1.263 at y -1.51
        # (rises toward the seats, 9.4 degrees, no roll; measured from the source OBJ faces
        # with normal z > 0.9 in the footprint). The earlier "z 1.20 to 1.23, 3.9 degrees"
        # was the BODY tube tops under the mesh. battery_car.p3d ODOL header: origin at the
        # base (geometry y -0.013 to about 0.19), so the origin goes ON the mesh: at y -1.60
        # the mesh is z 1.248, plus 0.013 along the normal, 1.26. 1.205 had the base 5 cm in.
        # build 52 (Jeff, build 51: "almost perfect, lower by about 1 more inch"): 1.235
        "carbattery": (0.0, -1.60, 1.235),
        "sparkplug": (0.0, rear_y + 0.9, 0.85),
        # build 49 (NIMBY-T-103): the light bar housing is 3 cm deep (y 1.987 to 2.015) and the bulb
        # model protruded through the lenses at its centre; 7 cm back puts the bulb inside
        "reflector_1_1": (lights["center"][0] - 0.3, lights["center"][1] - 0.07, lights["center"][2]),
        "reflector_2_1": (lights["center"][0] + 0.3, lights["center"][1] - 0.07, lights["center"][2]),
    }
    # Rail slots, measured from the source mesh (November 6 and 7, 2026). Rear deck: tube tops
    # at z 1.20 to 1.23 with the mesh panel above them rising from z 1.21 (y -1.81) to 1.26
    # (y -1.51), x -0.34 to 0.34, y -1.95 to -1.47, a cross tube above it at y -1.86 to
    # -1.81, z 1.33. Side rails: tub edge x 0.73 to 0.78 with the side tube at z 0.8 to 1.0,
    # seats at y -0.10, suspension arms outboard (x 1.06) at y -0.1 and 0.5 but only to z 0.7.
    # Build 37 put two jerry cans on the deck: slots bind and the vanilla debug fill attached
    # them, but the can sat a quarter of its height into the deck (its origin is about 0.12 m
    # above its base) and Jeff wants the cans beside the seats and the ammo cans at the rear.
    # Build 38 hung the cans against the side tube turned 90 degrees (forward along x): they
    # rendered lying flat on the mesh side shelf (the shelf is at z about 0.50, outboard of the
    # tub to the wheel line). Build 39 (Jeff, November 7, 09:53: standing, at one end of the
    # rail): the deck orientation of build 37, which stood the can upright, at the rear end of
    # the shelf behind the seats, base on the shelf; ammo cans on the deck as build 38.
    # Build 40 (Jeff, 10:13): with the deck triangle the ammo box rendered upside down and
    # the can lying on its broad side, cap to the rear: JerryCan.p3d is authored lying flat
    # (its cap along model forward, broad faces along model up) and the add_proxy "identity"
    # carries a 180 degree roll a wheel cannot show. Box: up negated. Can: model forward to
    # world up (standing on its end), model up along x (broad face against the tub wall).
    # Origins treated as model centres: box 0.19 tall on the deck at 1.23, can 0.47 tall on
    # the shelf at 0.50, 0.17 thick against the wall at x 0.78.
    # Build 47 (Jeff, 11:34): all storage on the side rails. 1 and 2 at the rear end of the
    # shelf behind the seats (build 45 pass), 3 and 4 at the front end toward the front wheels
    # (shelf z 0.47 to 0.50 runs y -0.74 to 1.12; tub wall x about 0.70 there). Every slot takes
    # a jerry can, an ammo can or a backpack, all drawn in the same frame: up to world up, the
    # item's forward axis inboard (outboard since build 52, see rail_up).
    # build 48: all four down 2 inches (Jeff, 11:59)
    # build 49: rear slots 2 inches forward, front slots 1 inch back (Jeff, 12:22); the ammo
    # box's extra 4 inch drop is per item, config.cpp AmmoBox InventorySlotsOffsets
    # Build 52: the drawn item sits with its model origin at the proxy and nothing in config moves
    # it (build 51 diag: InventorySlotsOffsets position[] moved the entity, not the picture;
    # orientation[] nothing). ODOL headers: JerryCan.p3d origin 0.235 above its base (flush on the
    # z 0.50 shelf from 0.685), 300RoundBox.p3d 0.10 (floated 8.5 cm, Jeff's "3 inches"),
    # hunting_g.p3d 0.0 (floated 18.5 cm, Jeff's "foot"). One frame per slot, so the front pair
    # is tuned for the base-origin items (ammo cans toward the front tyres, Jeff, build 46):
    # Build 52 tried z 0.55 for the front pair (box 5 cm into the mesh, Jeff: "mesh going through
    # the bottoms, raise about 3 inches"); build 53: 0.625, box base 2.5 cm above the mesh plane.
    # Rear pair stays at 0.685 for the jerry cans. Backpacks have their own slots (railbag_pos).
    rail_pos = {"nimbydpvrail_1": (-0.865, -0.50, 0.685), "nimbydpvrail_2": (0.865, -0.50, 0.685),
                "nimbydpvrail_3": (-0.82, 0.875, 0.625), "nimbydpvrail_4": (0.82, 0.875, 0.625)}
    # Build 53 backpack twins: same x and y as the rail slot (front pair 2 inches aft, Jeff,
    # build 52), z 0.55 (the bag's origin is at its base; build 52 drew it there and Jeff asked
    # only for the aft shift), item forward OUTBOARD so the bag's back shows (build 52).
    railbag_pos = {"nimbydpvrailbag_1": (-0.865, -0.50, 0.55), "nimbydpvrailbag_2": (0.865, -0.50, 0.55),
                   "nimbydpvrailbag_3": (-0.82, 0.825, 0.55), "nimbydpvrailbag_4": (0.82, 0.825, 0.55)}
    railbag_up = {"nimbydpvrailbag_1": (-1, 0, 0), "nimbydpvrailbag_2": (1, 0, 0),
                  "nimbydpvrailbag_3": (-1, 0, 0), "nimbydpvrailbag_4": (1, 0, 0)}
    # Build 55 tent slots: shelf centre (y -0.74 to 1.12 gives 0.19), x on the rear pair line,
    # z 0.55 (packed tents are base-origin bags as far as the inventory icons show; unproven).
    # First frame: item up to world up, item forward outboard like the bags. The packed tent's
    # long axis in its own model is unknown (the item P3D holds the pitched tent too, so the
    # ODOL header cannot say); the build 55 drive calibrates this frame.
    railtent_pos = {"nimbydpvrailtent_l": (-0.865, 0.19, 0.55), "nimbydpvrailtent_r": (0.865, 0.19, 0.55)}
    railtent_up = {"nimbydpvrailtent_l": (-1, 0, 0), "nimbydpvrailtent_r": (1, 0, 0)}
    # Build 50: front pair back 0.05 (Jeff: the front can "needs to come back about two inches").
    # Build 45 rendered (the rail_N bones in model.cfg were the missing piece): cans pass; the
    # ammo cans sat about 3 inches low with their labels forward. Build 46: boxes up 0.075,
    # turned 180 degrees (labels to the rear), moved back and outboard to clear the battery
    # that now stands behind the rear seat (part_pos carbattery).
    # Builds 40 to 42 drew nothing: the proxy classes were ProxyInventory (class-model) proxies,
    # not ProxyAttachment (build 43, config.cpp CfgNonAIVehicles). Proxy frame rule (DayZ Modding
    # Knowledge Pack, dayz-proxy-align: angle-sort, engine identity P' = [[-1,0,0],[0,0,1],[0,1,0]]):
    # the SHORT leg (add_proxy "up") is where the attached item's FORWARD axis points and the LONG
    # leg (add_proxy "forward") where its UP axis points. Build 39's triangle (short up, long back)
    # therefore laid the can on its broad face with the cap to the rear and the box lid-backward,
    # which is what Jeff saw. JerryCan.p3d is authored upright, cap +Y, broad faces +-Z.
    # Box: up to world up, latch side to the rear. Can: up to world up, broad face along x (flat
    # against the tub wall), mirrored per side. Origins taken as model centres.
    rail_fwd = {"nimbydpvrail_1": (0, 0, 1), "nimbydpvrail_2": (0, 0, 1),
                "nimbydpvrail_3": (0, 0, 1), "nimbydpvrail_4": (0, 0, 1)}
    # Item forward INBOARD for the can and box slots (builds 47 to 51; build 52 turned them
    # outboard for the bags and Jeff sent the ammo can back: "flip 180"). Bags live on the
    # railbag slots, whose frame is outboard (railbag_up).
    rail_up = {"nimbydpvrail_1": (1, 0, 0), "nimbydpvrail_2": (-1, 0, 0),
               "nimbydpvrail_3": (1, 0, 0), "nimbydpvrail_4": (-1, 0, 0)}
    report = {"half_width": half_w, "front_y": front_y, "rear_y": rear_y, "top_z": top_z, "rail": rail_pos, "railbag": railbag_pos, "railtent": railtent_pos,
              "crew_driver": crew_origin(seat_d), "crew_codriver": crew_origin(seat_c),
              "crew_cargo1": crew_origin(seat_r), "spare": spare, "engine": engine}

    lods = []
    # ---- visual LOD 0
    lod0 = Lod(RES_VISUAL[0], "visual-1")
    is_body = lambda r: r == "body" or r in ("drivewheel", "seat_driver", "seat_codriver", "seat_cargo1", "lights") or (isinstance(r, tuple) and r[0] == "disc")
    build_visual(verts, uvs, faces, parts, roles, lod0, is_body)
    add_antenna(lod0)
    add_wheel_post(lod0, spare)
    def add_vehicle_proxies(lod, with_crew=True, with_wheels=True, with_parts=True):
        n = 1
        if with_wheels:
            for name in wheel_names:
                c = W[name]["center"]
                # one side gets the 180 degree yaw the vanilla ADA stores for its right wheels, so
                # the rim face points outboard on both sides. The DPV wheel item's rim face is on
                # its +x side (in game on November 6, 2026 the identity side showed the dish), so
                # the yaw goes on the left (x < 0) wheels
                # ADA convention: identity on the engine-right wheels (P3D -x = working +x),
                # 180 degree yaw on the engine-left ones
                fwd = (0, -1, 0) if c[0] > 0 else (0, 1, 0)
                lod.add_proxy(WHEEL_PROXY, n, c, forward=fwd, sel=(name,)); n += 1
            # spare: identity forward rolled 90 degrees about it, so the axle is vertical and the
            # tyre lies flat; up = (-1, 0, 0) puts the rim face up (up = (1, 0, 0) put it face
            # down, build 21)
            lod.add_proxy(WHEEL_PROXY, n, spare, forward=(0, -1, 0), up=(-1, 0, 0), sel=("wheel_spare_1",))
        if with_crew:
            lod.add_proxy(CREW_DRIVER_PROXY, 1, crew_origin(seat_d), forward=(0, -1, 0), sel=("crewdriver",), crew=True)
            lod.add_proxy(CREW_CARGO_PROXY, 1, crew_origin(seat_c), forward=(0, -1, 0), sel=("crewcodriver",), crew=True)
            lod.add_proxy(CREW_CARGO_PROXY, 2, crew_origin(seat_r), forward=(0, -1, 0), sel=("crewcargo1",), crew=True)
        if with_parts:
            # battery flat, terminals up, front face to the rear (build 47; it stood on end with the
            # default triangle, which points the item's up axis backward, see the rail note)
            # build 49: label was north, so the item forward flips to -x; plus 10 degrees of roll
            # (left side down) on top of the 25 degree pitch, both from Jeff's in-game reading
            # Build 53 (Jeff, build 52): the battery is required but not drawn, like the spark
            # plug. No proxy here: the carbattery slot, its memory point and CarScript's
            # IsVitalCarBattery check are untouched, so the engine still needs one and the
            # inventory screen still takes it; nothing shows on the deck. Builds 46 to 52 drew it
            # on the rear deck (part_pos carbattery; frame history in the comments above).
            lod.add_proxy(PART_PROXIES["sparkplug"], 1, part_pos["sparkplug"], forward=(0, -1, 0), sel=("sparkplug",))
            lod.add_proxy(PART_PROXIES["reflector_1_1"], 1, part_pos["reflector_1_1"], forward=(0, -1, 0), sel=("reflector_1_1",))
            lod.add_proxy(PART_PROXIES["reflector_2_1"], 2, part_pos["reflector_2_1"], forward=(0, -1, 0), sel=("reflector_2_1",))
            for i, slot in enumerate(RAIL_SLOTS, 1):
                lod.add_proxy(RAIL_PROXY[slot], i, rail_pos[slot], forward=rail_fwd[slot], up=rail_up[slot], sel=("rail_%d" % i,))
            for i, slot in enumerate(RAILBAG_SLOTS, 1):
                lod.add_proxy(RAIL_PROXY[slot], 4 + i, railbag_pos[slot], forward=(0, 0, 1), up=railbag_up[slot], sel=("railbag_%d" % i,))
            for i, slot in enumerate(RAILTENT_SLOTS, 1):
                lod.add_proxy(RAIL_PROXY[slot], 8 + i, railtent_pos[slot], forward=(0, 0, 1), up=railtent_up[slot], sel=("railtent_%s" % "lr"[i - 1],))
    add_vehicle_proxies(lod0)
    lod0.properties.append(("lodnoshadow", "1"))
    lods.append(lod0)
    # ---- visual LODs 1 and 2
    if use_blender:
        for ratio, res in ((0.5, RES_VISUAL[1]), (0.25, RES_VISUAL[2])):
            l = decimate_lod(lod0, ratio, res, "visual-%g" % res)
            add_vehicle_proxies(l, with_crew=True, with_parts=False)
            l.properties.append(("lodnoshadow", "1"))
            lods.append(l)
    def part_slot_positions():
        # attachment slot name -> model position; the numbers add_vehicle_proxies uses
        return dict(part_pos)

    # ---- Geometry
    geo = Lod(RES_GEOMETRY, "geometry")
    per_wheel = 60.0
    hub_mass = per_wheel * 4
    # Collision follows the frame bottom measured from the source mesh (November 6, 2026):
    # floor 0.46 to 0.50 between the axles, tail 0.48 to 0.50 behind the rear axle rising to
    # 0.73 at the bumper, front 0.47 to the light bar. Builds 1 to 23 used one box from z
    # 0.30 over the whole length: 16 cm below the real floor and 18 cm below the tail, so the
    # car scraped a shape that is not there on every bump, and each scrape fed the Back zone
    # and through it the engine and its 20-hitpoint spark plug. The axle regions are covered
    # by the wheel hub cylinders below. Mass sits in the tub and the rear engine (the real
    # vehicle is rear-engined) with 10 per cent in the cage: a quarter of the mass above z 1.0
    # put the centre of mass far too high and the car swerved on every gear change (build 23).
    body_mass = TOTAL_MASS_KG - hub_mass
    geo.add_box((-0.80, -1.10, 0.46), (0.80, 1.40, 1.00), sel=("component01",), mass=body_mass * 0.55)        # tub
    geo.add_box((-0.55, -0.60, 1.50), (0.55, 0.80, min(top_z, 2.10)), sel=("component02",), mass=body_mass * 0.10)  # cage
    geo.add_box((-0.80, rear_y + 0.15, 0.50), (0.80, -1.10, 1.00), sel=("component03",), mass=body_mass * 0.25)  # tail, engine
    geo.add_box((-0.80, 1.40, 0.50), (0.80, front_y - 0.25, 1.00), sel=("component04",), mass=body_mass * 0.10)  # hood, front
    k = 5
    for name in wheel_names:
        c = W[name]["center"]
        # wheelHub="wheel_x_y_damper_land" in the config names this Geometry component; without it the
        # engine logs "Won't simulate, wheel wheel_1_1_damper_land has no proper selection in geometry"
        # and refuses to create the vehicle (RPT 2026-11-05 20:04). An 8-point box of that name was
        # found but the wheel sim still never initialised (NIMBY-T-069: present=0, every hub at
        # <0,0,0>, attach timing ruled out November 6, 2026 08:12). Both working references carry a
        # 24-point cylinder here, so the hub is now the Quad's shape: 12 segments about the spin axis,
        # radius = config wheelHubRadius 0.16, width 0.29, also selected as wheel_x_y_hide like the Quad.
        geo.add_cylinder(c, 0.16, 0.145, segments=12, sel=(name + "_damper_land", name + "_hide", "component%02d" % k), mass=per_wheel); k += 1
    geo.properties += [("class", "vehicle"), ("autocenter", "0"), ("lodnoshadow", "1")]
    lods.append(geo)
    # ---- View Geometry (crew proxies live here per the vehicle configuration page)
    vg = Lod(RES_VIEWGEOMETRY, "viewgeometry")
    # An open buggy, so the envelope is hollow where the seats are: the tub only to z 0.62
    # (floor at 0.47 to 0.60 in the source mesh, seat bottoms at 0.55), the hood deck and
    # the rear engine box higher, and the cage only above 1.5. The seat cushions (0.55 to
    # 0.85 front, 1.11 to 1.41 rear) stand clear of every other component, so the get-in
    # and switch-seat aim rays reach them. Builds 1 to 21 used one box to z 1.0 along the
    # whole tub plus a cage box from 1.0: with build 20's cushion-sized seat selections the
    # front cushions were inside the tub box and the rear cushion inside the cage box.
    vg.add_box((-0.80, rear_y + 0.15, 0.30), (0.80, front_y - 0.25, 0.62), sel=("component01",))
    vg.add_box((-0.55, -0.60, 1.50), (0.55, 0.80, min(top_z, 2.10)), sel=("component02",))
    # The vanilla ADA's View Geometry carries every attachment proxy plus a selection named after
    # each inventory SLOT (nivawheel_1_1 .. nivawheel_spare_1, carbattery, sparkplug,
    # reflector_1_1, reflector_2_1; read from offroadhatchback.p3d on 2026-11-05); its Memory LOD
    # has a point of the same slot names. Without the slot selections the wheel simulation never
    # sees the attached wheels (Diag: wheels=4 present=0 while four wheels sat in the slots).
    add_vehicle_proxies(vg, with_crew=True, with_wheels=True, with_parts=True)
    # Seat selections are the cushion only (0.30 m above the seat bottom), as on the ADA, not
    # the whole seat mesh: a box that reaches head height puts a seated player's camera inside
    # their own seat's selection, so the aim ray for ActionSwitchSeats hit the occupied seat
    # and the switch was never offered (build 19, November 6, 2026).
    def cushion(seat):
        lo = seat["lo"]; hi = seat["hi"]
        return lo, (hi[0], hi[1], lo[2] + 0.30)
    vg.add_box(*cushion(seat_d), sel=("seat_driver", "component03"))
    vg.add_box(*cushion(seat_c), sel=("seat_codriver", "component04"))
    vg.add_box(*cushion(seat_r), sel=("seat_cargo1", "component05"))
    vg.add_box((-0.6, rear_y + 0.1, 0.9), (0.6, rear_y + 0.9, 1.3), sel=("carcargoaccess", "component06"))
    vg.add_box((-0.9, rear_y + 0.7, 0.7), (-0.6, rear_y + 1.0, 1.0), sel=("refill", "component07"))
    vk = 8
    for name, slot in zip(wheel_names, WHEEL_SLOTS):
        c = W[name]["center"]; r = W[name]["radius"] * 0.8; hw = W[name]["width"] / 2
        vg.add_box((c[0] - hw, c[1] - r, c[2] - r), (c[0] + hw, c[1] + r, c[2] + r), sel=(slot, "component%02d" % vk)); vk += 1
    sr = W[wheel_names[2]]["radius"] * 0.8; sw = W[wheel_names[2]]["width"] / 2
    # flat tyre: radius in x and y, half width in z
    vg.add_box((spare[0] - sr, spare[1] - sr, spare[2] - sw), (spare[0] + sr, spare[1] + sr, spare[2] + sw), sel=(SPARE_SLOT, "component%02d" % vk)); vk += 1
    for slot, pos in part_slot_positions().items():
        vg.add_box((pos[0] - 0.12, pos[1] - 0.12, pos[2] - 0.12), (pos[0] + 0.12, pos[1] + 0.12, pos[2] + 0.12), sel=(slot, "component%02d" % vk)); vk += 1
    # Build 50: the rail interaction pad is a small slab on the shelf top (z 0.47 to 0.52), not a
    # 0.24 x 0.36 x 0.24 box around the proxy. The big box enclosed the ammo box entirely, so the
    # cursor raycast (View Geometry) hit the car first and the only prompt was "Push car"; the can
    # and the bag only worked because they poke out of it. Empty slot: look at the pad to attach.
    # Self-review, build 50: the pad stays outboard of the tub box (component01, x to 0.80, z to
    # 0.62) or its inner strip is buried under it; the front pair at x 0.82 is clipped to 0.805.
    # build 53: the pad carries both slot names of its position; ActionAttachOnSelection tries every
    # selection on the hit component (ActionAttachOnSelection.c:17-50), so a bag finds the railbag
    # slot and a can or box the rail slot from the same pad
    for slot, bagslot in zip(RAIL_SLOTS, RAILBAG_SLOTS):
        pos = rail_pos[slot]
        tentslot = RAILTENT_SLOTS[0] if pos[0] < 0 else RAILTENT_SLOTS[1]   # build 55: the side's tent slot too
        sx = 1.0 if pos[0] > 0 else -1.0
        xin = sx * max(abs(pos[0]) - 0.06, 0.805)
        xout = sx * (abs(pos[0]) + 0.06)
        vg.add_box((min(xin, xout), pos[1] - 0.10, 0.470), (max(xin, xout), pos[1] + 0.10, 0.520), sel=(slot, bagslot, tentslot, "component%02d" % vk)); vk += 1
    vg.add_box((-0.80, 0.80, 0.62), (0.80, front_y - 0.25, 0.95), sel=("component%02d" % vk,)); vk += 1       # hood deck
    vg.add_box((-0.80, rear_y + 0.15, 0.62), (0.80, -1.45, 1.00), sel=("component%02d" % vk,)); vk += 1      # rear engine box
    lods.append(vg)
    # ---- Fire Geometry: damage zones plus the wheel proxies the simulation attaches to
    fg = Lod(RES_FIREGEOMETRY, "firegeometry")
    fc = [1]
    def fbox(lo, hi, zone):
        fg.add_box(lo, hi, sel=(zone, "component%02d" % fc[0])); fc[0] += 1
    fbox((-0.80, -0.60, 0.30), (0.80, 1.10, 1.00), "dmgzone_chassis")
    fbox((-0.80, 1.10, 0.30), (0.80, front_y, 1.00), "dmgzone_front")
    fbox((-0.80, rear_y, 0.30), (0.80, -0.60, 1.00), "dmgzone_back")
    # engine zone from 0.60 up: below that the Back zone takes ground contact, so a scrape
    # under the tail reaches the engine only through Back's transfer coefficient
    fbox((-0.40, rear_y + 0.55, 0.60), (0.40, rear_y + 1.25, 0.95), "dmgzone_engine")
    fbox((-0.75, -1.20, 0.35), (-0.45, -0.40, 0.75), "dmgzone_fueltank")
    fbox((-0.55, -1.30, 1.00), (0.55, 0.80, min(top_z, 2.10)), "dmgzone_roof")
    for name, zone in zip(wheel_names, ("dmgzone_fender_1_1", "dmgzone_fender_2_1", "dmgzone_fender_1_2", "dmgzone_fender_2_2")):
        c = W[name]["center"]; r = W[name]["radius"]
        # The fender zone is the arch above the tyre, never a box around the hub: a Fire Geometry
        # convex that encloses the wheel proxy point stops the wheel simulation from creating
        # that wheel (NIMBY-T-069, November 6, 2026: the Quad body with the DPV's Fire Geometry
        # lost its wheels; the only Quad component around a hub is wheel_x_y_hide, which the
        # engine switches off while a wheel is attached). Box: 0.24 wide, 0.3 long, from just
        # above the tyre to 0.25 above it.
        fbox((c[0] - 0.12, c[1] - 0.15, c[2] + r + 0.02), (c[0] + 0.12, c[1] + 0.15, c[2] + r + 0.27), zone)
    lc = lights["center"]
    fbox((lc[0] - 0.5, lc[1] - 0.05, lc[2] - 0.12), (lc[0] - 0.1, lc[1] + 0.05, lc[2] + 0.12), "dmgzone_lights_1_1")
    fbox((lc[0] + 0.1, lc[1] - 0.05, lc[2] - 0.12), (lc[0] + 0.5, lc[1] + 0.05, lc[2] + 0.12), "dmgzone_lights_2_1")
    add_vehicle_proxies(fg, with_crew=False, with_wheels=True, with_parts=False)
    lods.append(fg)
    # ---- Memory
    mem = Lod(RES_MEMORY, "memory")
    def mp(name, p):
        mem.select_points(name, [mem.add_point(p)])
    for name in wheel_names:
        c = W[name]["center"]
        a = mem.add_point((c[0] - 0.3, c[1], c[2])); b = mem.add_point((c[0] + 0.3, c[1], c[2]))
        mem.select_points(name + "_axis", [a, b])
        # vertical axes at the hub: translation along it for the damper, rotation about it for steering.
        # The damper axis is a short span centred on the hub (0.09 m each way), as in the Arma2Quad
        # memory LOD; the 1.0 m span that started at the hub was another difference from both references.
        d0 = mem.add_point((c[0], c[1], c[2] - 0.09)); d1 = mem.add_point((c[0], c[1], c[2] + 0.09))
        mem.select_points(name + "_damper_axis", [d0, d1])
        if name.endswith("_1"):
            s0 = mem.add_point((c[0], c[1], c[2] - 0.5)); s1 = mem.add_point((c[0], c[1], c[2] + 0.5))
            mem.select_points(name + "_steering_axis", [s0, s1])
    sx = -1.0 if seat_d["center"][0] < 0 else 1.0
    mp("pos_driver", (sx * (half_w + 0.35), seat_d["center"][1], 0.5)); mp("pos_driver_dir", (sx * (half_w - 0.15), seat_d["center"][1], 0.5))
    mp("pos_codriver", (-sx * (half_w + 0.35), seat_c["center"][1], 0.5)); mp("pos_codriver_dir", (-sx * (half_w - 0.15), seat_c["center"][1], 0.5))
    mp("pos_cargo1", (-sx * (half_w + 0.35), seat_r["center"][1], 0.5)); mp("pos_cargo1_dir", (-sx * (half_w - 0.15), seat_r["center"][1], 0.5))
    # seat reach: CarScript.CanReachSeatFromDoors (CarScript.c:2177) measures the player's distance to
    # the seat's condition point (GetDoorConditionPointFromSelection); one per side plus the rear
    mp("seat_con_1_1", (sx * half_w, seat_d["center"][1], 0.5))
    mp("seat_con_2_1", (-sx * half_w, seat_c["center"][1], 0.5))
    # The rear seat sits on the centre line, so its condition point does too: with the point at
    # the passenger-side body edge, ActionGetInTransport.ActionCondition (1.0 m horizontal reach,
    # CarScript.c:2177) offered the seat from one spot beside the rear wheel and from nowhere
    # else (build 18, November 6, 2026). NimbyDpv_Car.CanReachSeatFromDoors widens the reach for
    # this seat so the three natural approaches (either side, behind) all qualify.
    mp("seat_con_1_2", (0.0, seat_r["center"][1], 0.5))
    # Build 49 (NIMBY-T-102, Jeff, November 7, 12:22: the wheel turned about an off-centre axis):
    # the rim's plane normal, fitted from the drivewheel part's vertices, is (0, 0.931, -0.366),
    # a column running forward and DOWN 21.5 degrees; the axis used until build 48 ran forward
    # and UP 18 degrees, so the wheel spun about an axis 40 degrees off its own. The rim centre
    # coincides with the part's bounding-box centre (both (-0.304, 0.35, 1.036)).
    dwc = dw["center"]
    dwn = (0.0, 0.931, -0.366)
    a = mem.add_point((dwc[0] - 0.3 * dwn[0], dwc[1] - 0.3 * dwn[1], dwc[2] - 0.3 * dwn[2])); b = mem.add_point((dwc[0] + 0.3 * dwn[0], dwc[1] + 0.3 * dwn[1], dwc[2] + 0.3 * dwn[2]))
    mem.select_points("axis_drivewheel", [a, b])
    mp("light_left", (lc[0] - 0.3, lc[1] + 0.02, lc[2])); mp("light_left_dir", (lc[0] - 0.3, lc[1] + 1.0, lc[2]))
    mp("light_right", (lc[0] + 0.3, lc[1] + 0.02, lc[2])); mp("light_right_dir", (lc[0] + 0.3, lc[1] + 1.0, lc[2]))
    mp("fuel", (-0.75, -0.8, 0.95)); mp("refill", (-0.9, rear_y + 0.85, 0.85))
    mp("ptcexhaust_start", (0.3, rear_y + 0.05, 0.45)); mp("ptcexhaust_end", (0.3, rear_y - 0.4, 0.4))
    mp("ptcenginepos", engine); mp("ptcdustpos", (0.0, 0.0, 0.2)); mp("ptccoolantpos", engine)  # read by CarScript at init; never played, no radiator
    mp("pos_carhorn", (0.0, front_y - 0.1, 0.8))
    mp("dmgzone_engine", engine)
    # slot-named memory points plus wheel_x_y_removing, as on the vanilla ADA (see View Geometry)
    for name, slot in zip(wheel_names, WHEEL_SLOTS):
        c = W[name]["center"]
        mp(slot, c)
        side = -1.0 if c[0] < 0 else 1.0
        mp(name + "_removing", (c[0] + side * 0.6, c[1], c[2]))
    mp(SPARE_SLOT, spare)
    for slot, pos in part_slot_positions().items():
        mp(slot, pos)
    for slot in RAIL_SLOTS:
        mp(slot, rail_pos[slot])
    for slot in RAILBAG_SLOTS:
        mp(slot, railbag_pos[slot])
    for slot in RAILTENT_SLOTS:
        mp(slot, railtent_pos[slot])
    lods.append(mem)
    # No LandContact LOD: neither the vanilla ADA (ODOL resolution table read 2026-11-05) nor the
    # working Arma2Quad 1.30 port carries one; a car's placement comes from the wheel simulation.
    # ---- No Hitpoints LOD (5e15). Neither the vanilla ADA ODOL (resolution table: 1..5, 1100,
    # 1e13, 1e15, 6e15, 7e15) nor the Arma2Quad carries one; DayZ's DamageSystem names Fire
    # Geometry selections (componentNames[]), and the DayZ engine's wheel simulation never
    # initialised while this LOD was present (NIMBY-T-069, November 6, 2026: the Quad body with
    # the DPV's View, Fire and Hitpoints LODs transplanted lost its wheels; every other LOD of the
    # DPV transplanted alone left the Quad driving). Kept behind a flag for the record.
    if not WRITE_HITPOINTS_LOD:
        return lods, report
    hp = Lod(RES_HITPOINTS, "hitpoints")
    def hpt(name, p):
        hp.select_points(name, [hp.add_point(p)])
    hpt("dmgzone_chassis", (0.0, 0.25, 0.65)); hpt("dmgzone_front", (0.0, 1.5, 0.65)); hpt("dmgzone_back", (0.0, -1.4, 0.65))
    hpt("dmgzone_engine", engine); hpt("dmgzone_fueltank", (-0.6, -0.8, 0.55)); hpt("dmgzone_roof", (0.0, -0.25, 1.6))
    for name, zone in zip(wheel_names, ("dmgzone_fender_1_1", "dmgzone_fender_2_1", "dmgzone_fender_1_2", "dmgzone_fender_2_2")):
        hpt(zone, W[name]["center"])
    hpt("dmgzone_lights_1_1", (lc[0] - 0.3, lc[1], lc[2])); hpt("dmgzone_lights_2_1", (lc[0] + 0.3, lc[1], lc[2]))
    lods.append(hp)
    return lods, report


# ----------------------------------------------------------------------------- the wheel
def build_wheel(verts, uvs, faces, parts, roles, facts):
    """Wheel item model from the rear-left wheel mesh plus its hub cap, centred on the
    wheel axis, axis along X, in the vehicle's frame orientation."""
    name = "wheel_1_2"
    c = facts["wheels"][name]["center"]
    idxs = [i for i, r in roles.items() if r == name or (isinstance(r, tuple) and r[0] == "hubcap" and r[1] == name)]
    shifted = [v_sub(p, c) for p in verts]
    lod = Lod(RES_VISUAL[0], "wheel-visual")
    build_visual(shifted, uvs, faces, parts, {i: roles[i] for i in idxs}, lod, lambda r: True, split_lights=False)
    lod.properties.append(("lodnoshadow", "1"))
    r = facts["wheels"][name]["radius"]; w = facts["wheels"][name]["width"]
    geo = Lod(RES_GEOMETRY, "wheel-geometry")
    geo.add_box((-w / 2, -r * 0.8, -r * 0.8), (w / 2, r * 0.8, r * 0.8), sel=("component01",), mass=WHEEL_MASS_KG)
    # View and Fire Geometry with the same component, as the Arma2Quad 1.30 wheel carries
    # (its wheel p3d: visual, Geometry, Memory, View Geometry, Fire Geometry)
    vgw = Lod(RES_VIEWGEOMETRY, "wheel-viewgeometry")
    vgw.add_box((-w / 2, -r * 0.8, -r * 0.8), (w / 2, r * 0.8, r * 0.8), sel=("component01",))
    vgw.properties += [("autocenter", "0"), ("lodnoshadow", "1")]
    fgw = Lod(RES_FIREGEOMETRY, "wheel-firegeometry")
    fgw.add_box((-w / 2, -r * 0.8, -r * 0.8), (w / 2, r * 0.8, r * 0.8), sel=("component01",))
    fgw.properties += [("autocenter", "0"), ("lodnoshadow", "1")]
    # no "class" property: the vanilla nivawheel.p3d carries only autocenter, drawimportance and
    # lodnoshadow, and the engine logs "Unknown object class 'vehiclepart'" for the old value
    geo.properties += [("autocenter", "0")]
    mem = Lod(RES_MEMORY, "wheel-memory")
    mem.select_points("axis", [mem.add_point((-0.3, 0, 0)), mem.add_point((0.3, 0, 0))])
    # the vanilla and Arma2Quad wheel memory points (item centre, bounds, inventory view side)
    mem.select_points("ce_center", [mem.add_point((0, 0, 0))])
    mem.select_points("ce_radius", [mem.add_point((w / 2, r, r))])
    mem.select_points("boundingbox_min", [mem.add_point((-w / 2, -r, -r))])
    mem.select_points("boundingbox_max", [mem.add_point((w / 2, r, r))])
    mem.select_points("invview", [mem.add_point((w / 2 + 0.05, 0, 0))])
    return [lod, geo, mem, vgw, fgw]


# ----------------------------------------------------------------------------- rail proxy model
def build_rail_proxy():
    """Minimal proxy p3d for the rail slots: one tiny untextured triangle in a visual LOD
    (selection "zbytek", as the vanilla proxy models carry) plus a Memory LOD with ce_center.
    The engine draws the attached item in place of it; the triangle itself is never seen."""
    lod = Lod(RES_VISUAL[0], "rail-visual")
    ids = [lod.add_point((0.0, 0.0, 0.0)), lod.add_point((0.01, 0.0, 0.0)), lod.add_point((0.0, 0.0, 0.01))]
    lod.add_face(ids, [(0.0, -1.0, 0.0)] * 3, [(0.0, 0.0)] * 3, sel=("zbytek",))
    lod.properties.append(("lodnoshadow", "1"))
    mem = Lod(RES_MEMORY, "rail-memory")
    mem.select_points("ce_center", [mem.add_point((0.0, 0.0, 0.0))])
    return [lod, mem]


# ----------------------------------------------------------------------------- main
def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--out", required=True)
    ap.add_argument("--no-blender", action="store_true")
    args = ap.parse_args(argv)
    use_blender = not args.no_blender
    if use_blender:
        try:
            import bpy  # noqa
        except ImportError:
            print("bpy not available; building without the decimated LODs")
            use_blender = False
    verts_cm, uvs, faces = parse_obj(args.source)
    verts = transform(verts_cm)
    parts = loose_parts(len(verts), faces)
    roles, facts = classify(verts, parts)
    os.makedirs(args.out, exist_ok=True)
    lods, report = build_vehicle(verts, uvs, faces, parts, roles, facts, use_blender)
    size = write_mlod(os.path.join(args.out, "dpv.p3d"), lods)
    wlods = build_wheel(verts, uvs, faces, parts, roles, facts)
    wsize = write_mlod(os.path.join(args.out, "dpvwheel.p3d"), wlods)
    rsize = write_mlod(os.path.join(args.out, "dpvrail.p3d"), build_rail_proxy())
    role_counts = Counter(r if isinstance(r, str) else r[0] for r in roles.values())
    report.update({
        "source": os.path.abspath(args.source),
        "vertices": len(verts), "faces": len(faces), "loose_parts": len(parts),
        "roles": dict(role_counts), "wheels": facts["wheels"],
        "lods": [{"name": l.name, "resolution": l.resolution, "points": len(l.points), "faces": len(l.faces),
                  "selections": sorted(l.selections.keys())} for l in lods],
        "wheel_lods": [{"name": l.name, "points": len(l.points), "faces": len(l.faces)} for l in wlods],
        "bytes": {"dpv.p3d": size, "dpvwheel.p3d": wsize, "dpvrail.p3d": rsize},
        "blender": use_blender,
    })
    with open(os.path.join(args.out, "build-report.json"), "w") as fh:
        json.dump(report, fh, indent=2, default=lambda o: list(o) if isinstance(o, (set, tuple)) else str(o))
    print("dpv.p3d %d bytes, %d LODs; dpvwheel.p3d %d bytes" % (size, len(lods), wsize))
    for l in lods:
        print("  %-14s res=%-8g points=%-6d faces=%-6d selections=%d" % (l.name, l.resolution, len(l.points), len(l.faces), len(l.selections)))


if __name__ == "__main__":
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else sys.argv[1:]
    main(argv)
