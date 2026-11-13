#!/usr/bin/env python3
"""Replace unreliable GPU-converted ASTC previews with deterministic CPU decoding.

Run after export and BEFORE validate_rmasset.py / Unity import. Requires Pillow,
numpy and texture2ddecoder. Raw evidence is never changed. Original images and
manifest are backed up outside the package. Only 2D, single-sample LDR ASTC is
supported; ambiguous footprints are rejected (or explicitly supply --block).
OpenGL raw rows are flipped to match RenderDoc SaveTexture image orientation.
"""
import argparse
import hashlib
import json
import shutil
from datetime import datetime
from pathlib import Path

import numpy as np
from PIL import Image
import texture2ddecoder

FOOTPRINTS = [(4,4),(5,4),(5,5),(6,5),(6,6),(8,5),(8,6),(8,8),
              (10,5),(10,6),(10,8),(10,10),(12,10),(12,12)]
def sha(data): return hashlib.sha256(data).hexdigest()

def repair(root, origin, block=None):
    root = Path(root).resolve()
    manifest_path = root / 'manifest.json'
    manifest = json.loads(manifest_path.read_text(encoding='utf-8'))
    backup = root.parent / ('ASTCRepairBackup-' + datetime.now().strftime('%Y%m%d-%H%M%S-%f'))
    plan = []
    # Preflight every resource before writing anything.
    for meta_path in sorted((root / 'assets/textures').glob('*/texture.json')):
        meta = json.loads(meta_path.read_text(encoding='utf-8'))
        if meta['format'] != 'ASTC_UNORM': continue
        if meta.get('depth',1) != 1 or meta.get('samples',1) != 1:
            raise ValueError('Unsupported ASTC depth/MSAA: ' + str(meta_path))
        resource_candidates = [b for b in FOOTPRINTS if all(
            ((max(1,meta['width']>>r['mip'])+b[0]-1)//b[0]) *
            ((max(1,meta['height']>>r['mip'])+b[1]-1)//b[1]) * 16 == r['bytes']
            for r in meta['rawSubresources'])]
        for image in meta['images']:
            mip, layer = image['mip'], image['layer']
            if mip < 0 or layer < 0: raise ValueError('Unsupported combined ASTC image')
            raw_ref = next(r for r in meta['rawSubresources']
                           if r['mip']==mip and r['layer']==layer and r['sample']==0)
            raw = (root / raw_ref['path']).read_bytes()
            if sha(raw) != Path(raw_ref['path']).stem:
                raise ValueError('Raw evidence SHA mismatch: ' + raw_ref['path'])
            w, h = max(1,meta['width']>>mip), max(1,meta['height']>>mip)
            candidates = [b for b in resource_candidates if ((w+b[0]-1)//b[0])*((h+b[1]-1)//b[1])*16 == len(raw)]
            if block:
                if block not in candidates: raise ValueError('Explicit footprint does not match byte length')
                footprint = block
            elif len(candidates)==1: footprint = candidates[0]
            else:
                # Tiny constant-color textures can have ambiguous footprints but
                # identical decoded pixels. Accept only that provable case.
                variants = [texture2ddecoder.decode_astc(raw,w,h,*b) for b in candidates]
                if not variants or any(v != variants[0] for v in variants[1:]):
                    raise ValueError('Ambiguous ASTC footprint for %s: %s; supply --block' % (meta_path,candidates))
                footprint = candidates[0]
            decoded = texture2ddecoder.decode_astc(raw,w,h,*footprint)
            im = Image.frombytes('RGBA',(w,h),decoded,'raw','BGRA')
            if origin == 'opengl': im = im.transpose(Image.Transpose.FLIP_TOP_BOTTOM)
            old = np.asarray(Image.open(root/image['path']).convert('RGBA'))
            new = np.asarray(im)
            row = dict(path=image['path'],eventId=meta['eventId'],resourceId=meta['resourceId'],
                       block=footprint,oldBlack=bool(old[:,:,:3].max()==0),
                       newBlack=bool(new[:,:,:3].max()==0),meanAbsoluteDifference=float(np.abs(old.astype(float)-new).mean()))
            plan.append((meta_path,meta,image,im,row))
    backup.mkdir(parents=True)
    shutil.copy2(manifest_path,backup/'manifest.json')
    touched = set()
    for meta_path,meta,image,im,row in plan:
        f = root/image['path']; dest = backup/image['path']; dest.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(f,dest)
        if meta_path not in touched:
            dest = backup/meta_path.relative_to(root); dest.parent.mkdir(parents=True,exist_ok=True); shutil.copy2(meta_path,dest)
        im.save(f)
        alpha = np.asarray(im)[:,:,3]
        image.update(alphaMin=int(alpha.min()),alphaMax=int(alpha.max()),
                     meaningfulAlpha=not (alpha.min()==alpha.max() and alpha.min() in (0,255)))
        meta['decodedImageRepair'] = {'method':'texture2ddecoder CPU ASTC LDR','origin':origin,
                                      'rawEvidenceModified':False}
        touched.add(meta_path)
    # Shared metadata may contain multiple image entries, all modified in place.
    for meta_path in touched:
        meta = next(item[1] for item in plan if item[0]==meta_path)
        meta_path.write_text(json.dumps(meta,indent=2)+'\n',encoding='utf-8')
    changed = {r[4]['path'] for r in plan} | {str(f.relative_to(root)).replace('\\','/') for f in touched}
    found = set()
    for entry in manifest['files']:
        if entry['path'] in changed:
            data = (root/entry['path']).read_bytes()
            entry['sha256'] = sha(data)
            # Schema uses bytes; do not invent a parallel size field.
            if 'bytes' in entry: entry['bytes'] = len(data)
            if 'size' in entry: entry['size'] = len(data)
            found.add(entry['path'])
    if found != changed: raise ValueError('Manifest missing repaired files: '+str(changed-found))
    manifest_path.write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
    report = {'package':str(root),'backup':str(backup),'decodedImages':len(plan),
              'blackToNonblack':sum(r[4]['oldBlack'] and not r[4]['newBlack'] for r in plan),
              'images':[r[4] for r in plan]}
    (backup/'RepairReport.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return report

if __name__ == '__main__':
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('package'); ap.add_argument('--origin',required=True,choices=['opengl','top-left'])
    ap.add_argument('--block',help='Explicit verified ASTC footprint, e.g. 6x6')
    args=ap.parse_args()
    block=tuple(map(int,args.block.split('x'))) if args.block else None
    result=repair(args.package,args.origin,block)
    print(json.dumps({k:v for k,v in result.items() if k!='images'},indent=2))
