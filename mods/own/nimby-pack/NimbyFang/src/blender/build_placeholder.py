# build_placeholder.py - Fang Phase 0 step 2: the three-bone placeholder snake, built headless.
#
# Builds the mesh, the armature (root, spine1, Head), three clips (idle, crawl, dead) as actions,
# saves the .blend, then exports through DayZ Animation Tools 1.0.0 (Mrtea101): the mesh and
# skeleton as one .txo (export_scene.txo, which Workbench imports to .xob) and each clip as a .txa
# (export_scene.txa with bSaveAll, one file per action). Specification:
# docs/plans/2026-11-02-fang-recon.md, "Placeholder specification for step 2". Plan 12 P3: this
# source commits, compiled binaries do not.
#
# Run (PowerShell 7), from anywhere:
#   & 'C:\Program Files\Blender Foundation\Blender 5.2\blender.exe' --background --python `
#       'D:\GitHub\nimby-badlands-server-wt\fang\mods\own\nimby-pack\NimbyFang\src\blender\build_placeholder.py' `
#       -- --out 'D:\GitHub\nimby-badlands-server-wt\fang\mods\own\nimby-pack\NimbyFang\src' [--discover-only]
#
# Console output is a few lines; the log is <out>\build_placeholder-<stamp>.log. --discover-only
# builds nothing and lists the add-on's operators and their properties.
#
# What the add-on's source (vendor/DayzAnimationTools, read November 2, 2026) decided here:
# - export_scene.txo exports every object in the file: the one armature as the skeleton, every
#   mesh as LOD 0. It writes no UVs (a TODO in the source), so texturing is a Workbench step.
#   By default it auto-creates a "pin_lookat" bone under "head"; recon R5 ruled no pin bone and
#   the AI template reads HeadLookBoneName = "Head", so bAutoCreateHeadLookBone is False here.
#   It adds an "EntityPosition" bone unless one exists (bEnsureEntityPosition, kept on: the
#   DayZ root convention).
# - export_scene.txa exports the active object's active action over action.frame_range, one
#   keyframe per frame per bone, and reads events from the action's pose markers named
#   "Name|UserString|UserInt". With bSaveAll it writes every action in the file to
#   <dir>\<ActionName>.txa. Clips are plain actions, not NLA strips: the exporter evaluates the
#   pose each frame, and unmuted NLA strips would blend into it.
# - Bone axes: the exporter rotates Blender's Y-along-the-bone frame, so a bone laid along world
#   +X with roll 0 has local Y = forward and local Z = up. Root translation and yaw use those.

import argparse
import datetime
import math
import os
import sys

import bpy
import bmesh
from mathutils import Vector

FPS = 30
BONES = (("root", 0.0, 0.5), ("spine1", 0.5, 0.92), ("Head", 0.92, 1.0))  # name, head x, tail x
TXO_KWARGS = dict(bAutoCreateHeadLookBone=False, bEnsureEntityPosition=True, fUnitScale=1.0)
TXA_KWARGS = dict(bSaveAll=True, eAnimType="FB", fpsOverride=0, fUnitScale=1.0,
                  bExportTranslationKeys=True, bExportRotationKeys=True, bExportScaleKeys=False)
IDLE_EVENTS = (("LookLeft", 20), ("LookFront", 40))  # recon R4, optional; name|userString|userInt
LOG = []


def log(line):
    stamp = datetime.datetime.now().strftime("%H:%M:%S")
    LOG.append(f"{stamp} {line}")


def flush_log(out_dir):
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    path = os.path.join(out_dir, f"build_placeholder-{stamp}.log")
    with open(path, "w", encoding="utf-8", newline="\r\n") as handle:
        handle.write("\n".join(LOG) + "\n")
    return path


def parse_args():
    argv = sys.argv[sys.argv.index("--") + 1:] if "--" in sys.argv else []
    parser = argparse.ArgumentParser()
    parser.add_argument("--out", required=True, help="NimbyFang/src folder; blender/, txo/, txa/ and textures/ are created under it")
    parser.add_argument("--discover-only", action="store_true")
    return parser.parse_args(argv)


def discover_operators():
    found = []
    for category in dir(bpy.ops):
        if category.startswith("_"):
            continue
        try:
            names = dir(getattr(bpy.ops, category))
        except Exception:
            continue
        for name in names:
            idname = f"{category}.{name}"
            if any(key in idname.lower() for key in ("txa", "txo", "dayz")):
                try:
                    rna = getattr(getattr(bpy.ops, category), name).get_rna_type()
                    props = [p.identifier for p in rna.properties if p.identifier not in ("rna_type",)]
                except Exception as error:
                    props = [f"<unreadable: {error}>"]
                found.append((idname, props))
    addon_keys = [k for k in bpy.context.preferences.addons.keys() if "dayz" in k.lower()]
    log(f"add-ons enabled matching dayz: {addon_keys}")
    for idname, props in found:
        log(f"operator {idname} properties={props}")
    if not found:
        log("no operator with txa, txo or dayz in its id; the add-on is not enabled in this Blender profile")
    return found


def clear_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    scene = bpy.context.scene
    scene.render.fps = FPS
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0


def tube_radius(x):
    # 0.02 m at the tail, 0.06 m at 0.3 m, tapering to 0.035 m where the head box starts.
    if x < 0.3:
        return 0.02 + (0.06 - 0.02) * (x / 0.3)
    return 0.06 - (0.06 - 0.035) * ((x - 0.3) / 0.62)


def build_mesh():
    # A tapering tube along +X (tail at 0, head at 0.92 m) plus a box head to 1.0 m. Z up, resting on z = 0.
    mesh = bpy.data.meshes.new("nimbysnake_mesh")
    bm = bmesh.new()
    segments, ring = 12, 6
    rings = []
    for i in range(segments + 1):
        x = i / segments * 0.92
        radius = tube_radius(x)
        rings.append([bm.verts.new((x, radius * math.cos(2 * math.pi * j / ring), radius + radius * math.sin(2 * math.pi * j / ring))) for j in range(ring)])
    bm.faces.new(list(reversed(rings[0])))
    bm.faces.new(rings[-1])
    for a, b in zip(rings, rings[1:]):
        for j in range(ring):
            bm.faces.new((a[j], a[(j + 1) % ring], b[(j + 1) % ring], b[j]))
    hx0, hx1, hy, hz = 0.92, 1.00, 0.035, 0.07
    c = [bm.verts.new((x, y, z)) for x in (hx0, hx1) for y in (-hy, hy) for z in (0.0, hz)]
    for quad in ((0, 1, 3, 2), (4, 6, 7, 5), (0, 2, 6, 4), (1, 5, 7, 3), (0, 4, 5, 1), (2, 3, 7, 6)):
        bm.faces.new([c[k] for k in quad])
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bmesh.ops.triangulate(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    material = bpy.data.materials.new("nimbysnake")
    material.diffuse_color = (0.33, 0.30, 0.16, 1.0)
    mesh.materials.append(material)
    obj = bpy.data.objects.new("nimbysnake", mesh)
    bpy.context.collection.objects.link(obj)
    log(f"mesh: {len(mesh.vertices)} vertices, {len(mesh.polygons)} triangles, material {material.name}")
    return obj


def build_texture(out_dir):
    # One flat 64 by 64 colour. The exporter writes no UVs, so this is Workbench input (to .paa), not a Blender mapping.
    tex_dir = os.path.join(out_dir, "textures")
    os.makedirs(tex_dir, exist_ok=True)
    image = bpy.data.images.new("nimbysnake_co", 64, 64, alpha=False)
    image.pixels = [0.33, 0.30, 0.16, 1.0] * (64 * 64)
    path = os.path.join(tex_dir, "nimbysnake_co.png")
    image.filepath_raw = path
    image.file_format = "PNG"
    image.save()
    log(f"texture: {path} ({os.path.getsize(path)} bytes)")


def build_armature():
    arm_data = bpy.data.armatures.new("nimbysnake_skeleton")
    arm = bpy.data.objects.new("nimbysnake_skeleton", arm_data)
    bpy.context.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    previous = None
    for name, head_x, tail_x in BONES:
        bone = arm_data.edit_bones.new(name)
        bone.head = Vector((head_x, 0.0, 0.03))
        bone.tail = Vector((tail_x, 0.0, 0.03))
        bone.roll = 0.0
        if previous is not None:
            bone.parent = previous
            bone.use_connect = True
        previous = bone
    bpy.ops.object.mode_set(mode="OBJECT")
    log(f"armature: {[b.name for b in arm_data.bones]}")
    return arm


def skin(mesh_obj, arm):
    # Hard skinning by X: each vertex belongs to the bone whose span holds it.
    for name, _, _ in BONES:
        mesh_obj.vertex_groups.new(name=name)
    counts = {name: 0 for name, _, _ in BONES}
    for vert in mesh_obj.data.vertices:
        x = vert.co.x
        for name, head_x, tail_x in BONES:
            if head_x <= x <= tail_x or (name == "Head" and x > tail_x):
                mesh_obj.vertex_groups[name].add([vert.index], 1.0, "REPLACE")
                counts[name] += 1
                break
    mesh_obj.parent = arm
    modifier = mesh_obj.modifiers.new("Armature", "ARMATURE")
    modifier.object = arm
    log(f"skin: vertices per bone {counts}")


def key_bone(arm, bone_name, frame, rot=(0.0, 0.0, 0.0), loc=None):
    # rot is (about local X, about local Y = along the bone, about local Z = up) in degrees.
    pose_bone = arm.pose.bones[bone_name]
    pose_bone.rotation_mode = "XYZ"
    pose_bone.rotation_euler = tuple(math.radians(r) for r in rot)
    pose_bone.keyframe_insert(data_path="rotation_euler", frame=frame)
    if loc is not None:
        pose_bone.location = loc
        pose_bone.keyframe_insert(data_path="location", frame=frame)


def new_action(arm, name):
    action = bpy.data.actions.new(name)
    action.use_fake_user = True
    if arm.animation_data is None:
        arm.animation_data_create()
    arm.animation_data.action = action
    return action


def finish_action(arm, action, frame_end, events=()):
    action.use_frame_range = True
    action.frame_start = 1
    action.frame_end = frame_end
    for name, frame in events:
        marker = action.pose_markers.new(f"{name}||0")
        marker.frame = frame
    slot = getattr(arm.animation_data, "action_slot", None)
    log(f"clip {action.name}: frames 1-{frame_end}, slot={getattr(slot, 'identifier', slot)}, events={[e[0] for e in events]}")
    arm.animation_data.action = None


def build_idle(arm):
    # 60 frames: Head yaws 15 degrees each way, spine1 5 degrees, root still.
    action = new_action(arm, "idle")
    for frame in range(1, 61):
        phase = 2 * math.pi * (frame - 1) / 60
        key_bone(arm, "Head", frame, rot=(0, 0, 15 * math.sin(phase)))
        key_bone(arm, "spine1", frame, rot=(0, 0, 5 * math.sin(phase)))
        key_bone(arm, "root", frame, loc=(0.0, 0.0, 0.0))
    finish_action(arm, action, 60, IDLE_EVENTS)


def build_crawl(arm):
    # 60 frames: lateral wave on spine1 and Head out of phase; root travels 0.4 m along the body (local Y = world +X).
    action = new_action(arm, "crawl")
    for frame in range(1, 61):
        t = (frame - 1) / 60
        phase = 2 * math.pi * t
        key_bone(arm, "spine1", frame, rot=(0, 0, 20 * math.sin(phase)))
        key_bone(arm, "Head", frame, rot=(0, 0, 20 * math.sin(phase + math.pi / 2)))
        key_bone(arm, "root", frame, loc=(0.0, 0.4 * t, 0.0))
    finish_action(arm, action, 60)


def build_dead(arm):
    # 10 frames: the whole body rolls 90 degrees about its length (root local Y) and holds.
    action = new_action(arm, "dead")
    for frame in range(1, 11):
        t = (frame - 1) / 9
        key_bone(arm, "root", frame, rot=(0, 90 * t, 0), loc=(0.0, 0.0, 0.0))
        key_bone(arm, "spine1", frame)
        key_bone(arm, "Head", frame)
    finish_action(arm, action, 10)


def select_only(arm):
    for obj in bpy.data.objects:
        obj.select_set(obj is arm)
    bpy.context.view_layer.objects.active = arm


def export_txo(arm, out_dir):
    txo_dir = os.path.join(out_dir, "txo")
    os.makedirs(txo_dir, exist_ok=True)
    path = os.path.join(txo_dir, "nimbysnake.txo")
    select_only(arm)
    result = bpy.ops.export_scene.txo(filepath=path, **TXO_KWARGS)
    size = os.path.getsize(path) if os.path.exists(path) else 0
    log(f"export txo: {result} -> {path} ({size} bytes)")
    if size:
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        for needle in ("root", "spine1", "Head", "EntityPosition", "pin_lookat", "nimbysnake"):
            log(f"  txo mentions {needle}: {needle in text}")
    return size > 0


def export_txa(arm, actions, out_dir):
    txa_dir = os.path.join(out_dir, "txa")
    os.makedirs(txa_dir, exist_ok=True)
    select_only(arm)
    arm.animation_data.action = actions[0]  # the exporter wants an active action even with bSaveAll
    result = bpy.ops.export_scene.txa(filepath=os.path.join(txa_dir, "all.txa"), **TXA_KWARGS)
    arm.animation_data.action = None
    log(f"export txa (bSaveAll): {result}")
    ok = True
    for action in actions:
        path = os.path.join(txa_dir, f"{action.name}.txa")
        if not os.path.exists(path):
            ok = False
            log(f"  {action.name}.txa MISSING")
            continue
        with open(path, "r", encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        frames = next((line.strip() for line in text.splitlines() if "#numFrames" in line), "?")
        fps = next((line.strip() for line in text.splitlines() if "#fps" in line), "?")
        log(f"  {action.name}.txa: {os.path.getsize(path)} bytes, {fps}, {frames}, events={'$events' in text}")
    return ok


def main():
    args = parse_args()
    os.makedirs(args.out, exist_ok=True)
    log(f"Blender {bpy.app.version_string}; out={args.out}")
    discover_operators()
    if args.discover_only:
        path = flush_log(args.out)
        print(f"Discovery only. Log: {path}")
        return
    clear_scene()
    mesh_obj = build_mesh()
    build_texture(args.out)
    arm = build_armature()
    skin(mesh_obj, arm)
    build_idle(arm)
    build_crawl(arm)
    build_dead(arm)
    actions = [bpy.data.actions[n] for n in ("idle", "crawl", "dead")]
    blend_dir = os.path.join(args.out, "blender")
    os.makedirs(blend_dir, exist_ok=True)
    blend_path = os.path.join(blend_dir, "nimbysnake_placeholder.blend")
    bpy.ops.wm.save_as_mainfile(filepath=blend_path)
    log(f"saved {blend_path} ({os.path.getsize(blend_path)} bytes)")
    txo_ok = export_txo(arm, args.out)
    txa_ok = export_txa(arm, actions, args.out)
    path = flush_log(args.out)
    print(f"Built: {len(mesh_obj.data.vertices)} vertices, {len(mesh_obj.data.polygons)} triangles, 3 bones, 3 clips. "
          f"txo {'OK' if txo_ok else 'FAILED'}, txa {'OK' if txa_ok else 'FAILED'}. Log: {path}")
    if not (txo_ok and txa_ok):
        sys.exit(2)


main()
