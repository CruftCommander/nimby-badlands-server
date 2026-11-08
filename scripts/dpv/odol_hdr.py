#!/usr/bin/env python3
# odol_hdr.py - reads the ODOL v56 header of a binarized DayZ P3D (no Blender): LOD count, whole-model
# bounding boxes, bounding/geometry centres and centre of mass. "origin above base" is -bboxMin.y, the
# number the rail and roof frames are tuned to (November 7, 2026).
#   python3 -I scripts/dpv/odol_hdr.py <file.p3d> [more.p3d ...]
import struct, sys
def hdr(path):
    b=open(path,'rb').read(); assert b[:4]==b'ODOL', b[:4]
    v,=struct.unpack_from('<I',b,4); pos=8
    if v>=58: pos+=4  # appId (not in DayZ v56)
    if v>=58:
        e=b.index(b'\0',pos); pos=e+1
    n,=struct.unpack_from('<I',b,pos); pos+=4; pos+=4*n
    special,bsph,gsph,rem,andh,orh=struct.unpack_from('<IffIII',b,pos); pos+=24
    aim=struct.unpack_from('<fff',b,pos); pos+=12
    pos+=8  # map colors
    vd,=struct.unpack_from('<f',b,pos); pos+=4
    bmin=struct.unpack_from('<fff',b,pos); pos+=12
    bmax=struct.unpack_from('<fff',b,pos); pos+=12
    if v>=70: pos+=4
    if v>=71: pos+=4
    if v>=52:
        vmin=struct.unpack_from('<fff',b,pos); pos+=12
        vmax=struct.unpack_from('<fff',b,pos); pos+=12
    bc=struct.unpack_from('<fff',b,pos); pos+=12
    gc=struct.unpack_from('<fff',b,pos); pos+=12
    com=struct.unpack_from('<fff',b,pos); pos+=12
    return v,n,bmin,bmax,vmin,vmax,bc,gc,com
for p in sys.argv[1:]:
    v,n,bmin,bmax,vmin,vmax,bc,gc,com=hdr(p)
    name=p.split('/')[-1]
    print(f"{name}: v{v} lods={n}")
    print(f"  bboxMin  {tuple(round(x,3) for x in bmin)}  bboxMax {tuple(round(x,3) for x in bmax)}")
    print(f"  visMin   {tuple(round(x,3) for x in vmin)}  visMax  {tuple(round(x,3) for x in vmax)}")
    print(f"  boundCtr {tuple(round(x,3) for x in bc)}  geomCtr {tuple(round(x,3) for x in gc)}  CoM {tuple(round(x,3) for x in com)}")
    print(f"  origin above base (-bboxMin.y) = {-bmin[1]:.3f}; height = {bmax[1]-bmin[1]:.3f}; width x = {bmax[0]-bmin[0]:.3f}; depth z = {bmax[2]-bmin[2]:.3f}")
