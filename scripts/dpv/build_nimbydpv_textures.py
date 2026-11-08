#!/usr/bin/env python3
# -----------------------------------------------------------------------------
# build_nimbydpv_textures.py - DPV texture derivation (Phase 1, M-3).
#
# Reads the three layered PSDs of the carbuni model (never committed) and writes the
# DayZ texture set plus the three rvmat files into <out>:
#
#   dpv_body_co.tga   2048  diffuse: BASE black (red and white hidden), ELEMENTS, BOLTS,
#                           DIRT, AO, Shadow, Highlights; the TEXT group (the CHENOWTH
#                           type layer and the gauges) is dropped (inventory D-11)
#   dpv_body_nohq.tga 2048  tangent-space normal map from the BUMP,HOLES and BOLTS layers
#   dpv_body_smdi.tga 2048  specular: R unused (white), G specular strength, B glossiness,
#                           from the metal, metal2, Reflection and Highlights layers, with
#                           fabric and seatbelts dull
#   dpv_wheel_co.tga  1024, dpv_wheel_nohq.tga (rubber_tread, bump1), dpv_wheel_smdi.tga
#   dpv_alpha_ca.tga  1024  RGBA for the mesh panels and spring sleeves (alpha from the
#                           layer pixels; where the PSD has none, from luminance)
#   dpv_body.rvmat, dpv_wheel.rvmat, dpv_alpha.rvmat   Super shader, copied from the ADA's
#                           engine_niva.rvmat layout with our texture paths
#
# Then Jeff runs DayZ Tools ImageToPAA on the TGAs (the script prints the commands).
#
#   python3 build_nimbydpv_textures.py --source <vendor-carbuni dir> --out <dir>
# Needs: pip install psd-tools pillow numpy
# -----------------------------------------------------------------------------
import argparse
import os
import sys

import numpy as np
from PIL import Image, ImageFilter
from psd_tools import PSDImage

PBO_PREFIX = "NimbyDpv"
DATA = PBO_PREFIX + r"\data"


def composite(psd, visible, hidden, size):
    """Composite the PSD with only the named layers (or groups) toggled per the lists."""
    for layer in psd.descendants():
        if layer.name in hidden:
            layer.visible = False
        elif layer.name in visible:
            layer.visible = True
    img = psd.composite(force=True)
    if img.mode != "RGBA":
        img = img.convert("RGBA")
    return img.resize((size, size), Image.LANCZOS)


def layer_lum(psd, names, size, mode="lum"):
    """0..1 float array from the given layers composited alone: mode "lum" is luminance
    times alpha (bright detail), "dark" is alpha times darkness (black line-work and
    recesses), "alpha" is coverage only. Missing layers contribute nothing."""
    acc = np.zeros((size, size), dtype=np.float32)
    for layer in psd.descendants():
        if layer.name in names and not layer.is_group():
            try:
                img = layer.composite()
            except Exception:
                continue
            if img is None:
                continue
            canvas = Image.new("RGBA", (psd.width, psd.height), (0, 0, 0, 0))
            canvas.paste(img, (layer.left, layer.top))
            canvas = canvas.resize((size, size), Image.LANCZOS)
            a = np.asarray(canvas, dtype=np.float32) / 255.0
            lum = 0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]
            if mode == "dark":
                val = a[..., 3] * (1.0 - lum)
            elif mode == "alpha":
                val = a[..., 3]
            else:
                val = lum * a[..., 3]
            acc = np.maximum(acc, val)
    return acc


def height_to_normal(h, strength):
    """Tangent-space normal map (DirectX green-down convention) from a height field."""
    h = np.asarray(Image.fromarray((h * 255).astype(np.uint8)).filter(ImageFilter.GaussianBlur(0.8)), dtype=np.float32) / 255.0
    dx = np.roll(h, -1, axis=1) - np.roll(h, 1, axis=1)
    dy = np.roll(h, -1, axis=0) - np.roll(h, 1, axis=0)
    nx = -dx * strength
    ny = dy * strength
    nz = np.ones_like(h)
    norm = np.sqrt(nx * nx + ny * ny + nz * nz)
    rgb = np.stack([(nx / norm + 1) * 0.5, (ny / norm + 1) * 0.5, (nz / norm + 1) * 0.5], axis=-1)
    return Image.fromarray((rgb * 255).astype(np.uint8), "RGB")


def smdi(spec, gloss):
    """R = 255 (unused), G = specular strength, B = glossiness; all 0..1 arrays."""
    r = np.full_like(spec, 1.0)
    rgb = np.stack([r, np.clip(spec, 0, 1), np.clip(gloss, 0, 1)], axis=-1)
    return Image.fromarray((rgb * 255).astype(np.uint8), "RGB")


def rvmat(path, base, spec, power, alpha=False, emissive=(0, 0, 0, 1)):
    stages = [
        'ambient[]={1,1,1,1};', 'diffuse[]={1,1,1,1};', 'forcedDiffuse[]={0,0,0,0};', 'emmisive[]={%s,%s,%s,%s};' % emissive,
        'specular[]={%s,%s,%s,1};' % (spec, spec, spec), 'specularPower=%d;' % power,
        'PixelShaderID="Super";', 'VertexShaderID="Super";',
    ]
    if alpha:
        stages += ['renderFlags[]={"AlphaTest32"};']
    tx = lambda s: '\tuvSource="tex";\n\tclass uvTransform\n\t{\n\t\taside[]={1,0,0};\n\t\tup[]={0,1,0};\n\t\tdir[]={0,0,0};\n\t\tpos[]={0,0,0};\n\t};'
    body = "\n".join(stages) + "\n"
    body += 'class Stage1\n{\n\ttexture="%s\\%s_nohq.paa";\n%s\n};\n' % (DATA, base, tx(0))
    body += 'class Stage2\n{\n\ttexture="#(argb,8,8,3)color(0.5,0.5,0.5,0.5,DT)";\n%s\n};\n' % tx(0)
    body += 'class Stage3\n{\n\ttexture="#(argb,8,8,3)color(0,0,0,0,MC)";\n%s\n};\n' % tx(0)
    body += 'class Stage4\n{\n\ttexture="#(argb,8,8,3)color(1,1,1,1,AS)";\n%s\n};\n' % tx(0)
    body += 'class Stage5\n{\n\ttexture="%s\\%s_smdi.paa";\n%s\n};\n' % (DATA, base, tx(0))
    body += 'class Stage6\n{\n\ttexture="#(ai,64,64,1)fresnel(0.7,0.8)";\n\tuvSource="none";\n};\n'
    body += 'class Stage7\n{\n\ttexture="dz\\data\\data\\env_land_co.paa";\n\tuvSource="none";\n};\n'
    with open(path, "w", newline="\r\n") as fh:
        fh.write(body)


def main(argv):
    ap = argparse.ArgumentParser()
    ap.add_argument("--source", required=True)
    ap.add_argument("--out", required=True)
    args = ap.parse_args(argv)
    os.makedirs(args.out, exist_ok=True)
    out = lambda n: os.path.join(args.out, n)

    # ---- body
    psd = PSDImage.open(os.path.join(args.source, "Desert_Patrol_Vehicle_texture.psd"))
    hidden = {"red", "white", "TEXT", "CHENOWTH", "gauges", "UV"}
    co = composite(psd, {"black", "BASE", "ELEMENTS", "DIRT", "BOLTS", "BUMP,HOLES", "AO", "Shadow", "Highlights", "Reflection", "Reflection2"}, hidden, 2048)
    co.convert("RGB").save(out("dpv_body_co.tga"))
    # recesses (black line-work, holes) go down, bolt heads go up; the PSD's bump layers are
    # thin, so a high-pass of the diffuse (panel seams, bolt shading, dirt edges) is added
    co_lum = np.asarray(co.convert("L").filter(ImageFilter.GaussianBlur(0.6)), dtype=np.float32) / 255.0
    co_blur = np.asarray(co.convert("L").filter(ImageFilter.GaussianBlur(6.0)), dtype=np.float32) / 255.0
    highpass = np.clip((co_lum - co_blur) * 2.5, -0.5, 0.5)
    h = 0.5 + 0.6 * highpass - 0.5 * layer_lum(psd, {"bump1", "lines_black"}, 2048, mode="dark") + 0.5 * layer_lum(psd, {"bolts1"}, 2048, mode="alpha")
    height_to_normal(np.clip(h, 0, 1), 3.0).save(out("dpv_body_nohq.tga"))
    metal = layer_lum(psd, {"metal", "metal2", "Reflection", "Reflection2"}, 2048)
    hi = layer_lum(psd, {"Highlights"}, 2048)
    dull = layer_lum(psd, {"fabric", "seatbelts"}, 2048)
    spec = np.clip(0.25 + 0.6 * metal + 0.4 * hi - 0.8 * dull, 0.05, 1.0)
    gloss = np.clip(0.35 + 0.5 * metal - 0.6 * dull, 0.05, 1.0)
    smdi(spec, gloss).save(out("dpv_body_smdi.tga"))
    rvmat(out("dpv_body.rvmat"), "dpv_body", 0.3, 400)
    # Build 63 (November 8, 2026, 00:49): the headlight "on" material, swapped onto the light_left /
    # light_right hidden selections by CarScript (frontReflectorMatOn). An rvmat's emissive is
    # multiplied by the face's diffuse texel, and the DPV's lens texels are near-black glass, so the
    # ADA's niva_lights.rvmat (1, 1, 0.75 at intensity 8, builds 18 to 62) lit the road and left the
    # lenses dark. Same body stages, emissive scaled up for the dark lenses; warm white.
    rvmat(out("dpv_lights_on.rvmat"), "dpv_body", 0.3, 400, emissive=(40, 38, 28, 1))

    # ---- wheel
    psd = PSDImage.open(os.path.join(args.source, "Desert_Patrol_Vehicle_wheels.psd"))
    co = composite(psd, {"black", "BASE", "ELEMENTS", "DIRT", "AO", "Highligths"}, {"UV"}, 1024)
    co.convert("RGB").save(out("dpv_wheel_co.tga"))
    h = 0.5 + 0.5 * layer_lum(psd, {"rubber_tread", "bolts"}, 1024, mode="alpha") - 0.5 * layer_lum(psd, {"bump1"}, 1024, mode="dark")
    height_to_normal(np.clip(h, 0, 1), 4.0).save(out("dpv_wheel_nohq.tga"))
    paint = layer_lum(psd, {"black_paint", "bolts"}, 1024)
    spec = np.clip(0.12 + 0.5 * paint, 0.05, 1.0); gloss = np.clip(0.2 + 0.5 * paint, 0.05, 1.0)
    smdi(spec, gloss).save(out("dpv_wheel_smdi.tga"))
    rvmat(out("dpv_wheel.rvmat"), "dpv_wheel", 0.15, 250)

    # ---- alpha (mesh panels, spring sleeves)
    psd = PSDImage.open(os.path.join(args.source, "Desert_Patrol_Vehicle_alpha.psd"))
    ca = composite(psd, {"black", "BASE", "ELEMENTS", "spring", "fence", "Highlights"}, set(), 1024)
    arr = np.asarray(ca, dtype=np.float32) / 255.0
    cover = layer_lum(psd, {"spring", "fence"}, 1024, mode="alpha")
    if cover.max() > 0.05:
        arr[..., 3] = np.clip(cover, 0, 1)        # the wire and spring pixels are opaque, the holes clear
    elif arr[..., 3].min() > 0.99:                # no layer alpha at all: fall back to luminance
        lum = 0.299 * arr[..., 0] + 0.587 * arr[..., 1] + 0.114 * arr[..., 2]
        arr[..., 3] = np.clip(lum * 2.0, 0, 1)
    Image.fromarray((arr * 255).astype(np.uint8), "RGBA").save(out("dpv_alpha_ca.tga"))
    flat = np.full((1024, 1024), 0.5, dtype=np.float32)
    height_to_normal(flat, 1.0).save(out("dpv_alpha_nohq.tga"))
    smdi(np.full((1024, 1024), 0.3, np.float32), np.full((1024, 1024), 0.3, np.float32)).save(out("dpv_alpha_smdi.tga"))
    rvmat(out("dpv_alpha.rvmat"), "dpv_alpha", 0.3, 300, alpha=True)

    print("written to", args.out)
    print("ImageToPAA (DayZ Tools), one per TGA, from the output folder:")
    for n in sorted(os.listdir(args.out)):
        if n.endswith(".tga"):
            print('  & "C:\\Program Files (x86)\\Steam\\steamapps\\common\\DayZ Tools\\Bin\\ImageToPAA\\ImageToPAA.exe" "%s"' % n)


if __name__ == "__main__":
    main(sys.argv[1:])
