"""Execute the embedded game and drawing code; artifacts stay in ignored build/."""
from pathlib import Path
import ctypes as c
import struct
import hashlib
from PIL import Image, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
lib=c.CDLL(str(ROOT/'build'/'host_tests.dll'))
for name in ('pea_tests','pea_sound_tests'):
    fn=getattr(lib,name)
    fn.restype=c.c_int
    result=fn()
    assert result==0, f'{name}: failed case {result}'
    print(f'PASS: {name}')
lib.pea_preview.argtypes=[c.c_int]
lib.pea_preview.restype=c.POINTER(c.c_uint16)
names=['PLAY','READY','PAUSE','GAME OVER','SETTINGS','STANDBY','BOOT','FREEZE']
montage=Image.new('RGB',(1440,900),(24,24,24))
draw=ImageDraw.Draw(montage)
for scene,name in enumerate(names):
    pointer=lib.pea_preview(scene)
    pixels=[pointer[i] for i in range(160*80)]
    assert sum(bool(p) for p in pixels)>300, f'blank scene {name}'
    rgb=[((p>>11)*255//31,((p>>5)&63)*255//63,(p&31)*255//31) for p in pixels]
    image=Image.new('RGB',(160,80))
    image.putdata(rgb)
    image.save(ROOT/'build'/f'game_{scene}.png')
    x,y=(scene%3)*480,(scene//3)*300
    montage.paste(image.resize((480,240),Image.Resampling.NEAREST),(x,y+30))
    draw.text((x+10,y+8),name,fill=(240,240,240))
montage.save(ROOT/'build'/'game_preview.png')
print('PASS: eight actual firmware scenes and pixel assets')
lib.pea_motion.argtypes=[c.c_int,c.c_int]
lib.pea_motion.restype=c.POINTER(c.c_uint16)
def motion(ticks,paused):
    pointer=lib.pea_motion(ticks,paused)
    return tuple(pointer[i] for i in range(160*80))
assert motion(0,0)!=motion(10,0), 'running game must move beans/enemies'
assert motion(0,1)==motion(10,1), 'paused game must not advance'
print('PASS: real game model moves on screen and stays still while paused')
firmware=ROOT/'build'/'main3.ino.bin'
if firmware.exists():
    binary=firmware.read_bytes()
    assert 0<len(binary)<=0x200000
    table=(ROOT/'build'/'main3.ino.partitions.bin').read_bytes()
    apps={}
    for offset in range(0,len(table)-31,32):
        magic,kind,subtype,address,size,label,flags=struct.unpack_from('<HBBII16sI',table,offset)
        if magic==0x50AA and kind==0:
            apps[label.split(b'\0')[0].decode()]=(address,size,flags)
    assert apps['app0']==(0x10000,0x200000,1)
    assert apps['app1']==(0x210000,0x200000,1)
    print(f'PASS: {len(binary)}B fits App0, existing encrypted partition layout retained')
    print('SHA256:',hashlib.sha256(binary).hexdigest())
