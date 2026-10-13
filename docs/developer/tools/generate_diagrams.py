"""Generate the public English interface diagrams. Requires Pillow.

Run: python docs/developer/tools/generate_diagrams.py
Outputs matching SVG/PNG pairs to docs/developer/assets.
This script contains diagram definitions, not manufacturer firmware source.
"""
from pathlib import Path
from html import escape
import math
from PIL import Image, ImageDraw, ImageFont

OUT = Path(__file__).resolve().parents[1] / "assets"
OUT.mkdir(parents=True, exist_ok=True)
FONT = next((p for p in [Path("C:/Windows/Fonts/segoeui.ttf"),
    Path("C:/Windows/Fonts/arial.ttf"),
    Path("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf")]
    if p.exists()), None)
INK, MUTED, BLUE, PURPLE, GREEN = "#172c45", "#526780", "#007fbd", "#7054c8", "#087e65"

class Canvas:
    def __init__(self, width, height, title, subtitle):
        self.im = Image.new("RGB", (width, height), "white")
        self.d = ImageDraw.Draw(self.im)
        self.svg = [f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}" role="img"><title>{escape(title)}</title><desc>{escape(subtitle)}</desc><rect width="100%" height="100%" fill="white"/>']
        self.text(44,25,title,36)
        self.text(44,80,subtitle,22,MUTED)
    def text(self,x,y,text,size=24,color=INK):
        font = ImageFont.truetype(str(FONT),size) if FONT else ImageFont.load_default()
        self.d.text((x,y),text,font=font,fill=color)
        self.svg.append(f'<text x="{x}" y="{y}" dominant-baseline="text-before-edge" fill="{color}" font-size="{size}" font-family="Segoe UI,Arial,DejaVu Sans,sans-serif">{escape(text)}</text>')
    def rect(self,x,y,w,h,fill="#f2f6fa",stroke=INK,r=12):
        self.d.rounded_rectangle((x,y,x+w,y+h),r,fill,stroke,2)
        self.svg.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" rx="{r}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')
    def circle(self,x,y,r,fill="white",stroke=INK):
        self.d.ellipse((x-r,y-r,x+r,y+r),fill,stroke,2)
        self.svg.append(f'<circle cx="{x}" cy="{y}" r="{r}" fill="{fill}" stroke="{stroke}" stroke-width="2"/>')
    def line(self,points,color=INK,width=3,arrow=False):
        self.d.line(points,fill=color,width=width)
        coords=" ".join(f"{x},{y}" for x,y in points)
        self.svg.append(f'<polyline points="{coords}" fill="none" stroke="{color}" stroke-width="{width}"/>')
        if arrow:
            x,y=points[-1];px,py=points[-2];a=math.atan2(y-py,x-px)
            tri=[(x,y),(x-14*math.cos(a-.45),y-14*math.sin(a-.45)),(x-14*math.cos(a+.45),y-14*math.sin(a+.45))]
            self.d.polygon(tri,fill=color)
            coords=" ".join(f"{x},{y}" for x,y in tri)
            self.svg.append(f'<polygon points="{coords}" fill="{color}"/>')
    def box(self,x,y,w,h,title,lines,color=BLUE):
        h=max(h,58+len(lines)*34+12)
        self.rect(x,y,w,h,stroke=color)
        self.text(x+20,y+14,title,27,color)
        for i,t in enumerate(lines):self.text(x+20,y+58+i*34,t,22)
    def save(self,name):
        self.svg.append('</svg>')
        (OUT/f'{name}.svg').write_text('\n'.join(self.svg),encoding='utf-8')
        self.im.save(OUT/f'{name}.png')

c=Canvas(1600,1400,"01  Front Controls and Hardware Interfaces","Upright view: logo at top, strings vertical. E = MPR121 electrode; GPIO = ESP32-S3 pin.")
c.rect(555,145,490,1130,"#edf2f6",r=35);c.text(588,170,"Fingersonic",34)
c.rect(580,235,175,180,"white");c.circle(666,320,50,"#dae5ed");c.circle(666,320,24,"#607c92")
c.rect(770,235,250,250,"white");c.circle(895,360,97,"#dceee9",GREEN);c.circle(895,360,59,"#becfd7")
c.rect(580,435,175,190,"#172c45");c.text(606,505,"160 x 80",23,"white")
c.rect(770,505,250,65,"white")
for y in range(520,560,12):
    for x in range(790,1010,17):c.circle(x,y,3,"#8798a5","#8798a5")
c.rect(770,590,123,58,"#e8f4ff",BLUE);c.rect(895,590,125,58,"#e8f4ff",BLUE)
c.text(793,604,"E2 Down",23,BLUE);c.text(919,604,"E5 Up",23,BLUE)
c.rect(580,675,440,355,"white")
for x in range(606,1010,78):c.line([(x,690),(x,1013)],"#90a5b5",7)
c.text(700,1050,"Six touch strings",23,MUTED)
c.rect(580,1110,219,90,"#f0ebff",PURPLE);c.rect(801,1110,219,90,"#f0ebff",PURPLE)
c.text(605,1120,"Tempo / E9",27,PURPLE);c.text(833,1120,"Meter / E8",27,PURPLE)
c.text(622,1162,"Left / O",22);c.text(851,1162,"Right / N",22)
c.box(44,240,390,142,"Encoder L",["A/CLK GPIO38; B/DT GPIO39","SW GPIO47; pressed LOW"],BLUE)
c.line([(434,311),(515,311),(616,320)],BLUE);c.circle(616,320,5,BLUE,BLUE)
c.box(1110,245,440,155,"Joystick G / LED ring F",["X GPIO6; Y GPIO5; SW GPIO7","16 LEDs; DIN GPIO48","Switch LOW; pixel order GRB"],GREEN)
c.line([(1110,315),(1070,315),(954,360)],GREEN);c.circle(954,360,5,GREEN,GREEN)
c.box(44,445,390,175,"Display M",["SCLK13; MOSI14; CS10","DC18; RESET21; BL9","ST7735S; landscape 160 x 80"],BLUE)
c.line([(434,530),(515,530),(580,530)],BLUE);c.circle(580,530,5,BLUE,BLUE)
c.box(1110,480,440,172,"Pitch / drum pads I, J",["Below speaker, above strings","Left Down / J / E2: Kick 36","Right Up / I / E5: Snare 38"],BLUE)
c.line([(1110,590),(1075,590),(1010,619)],BLUE);c.circle(1010,619,5,BLUE,BLUE)
c.box(44,745,390,194,"Touch strings K",["String 6 -> 1: E A D G B E","Electrodes: 1, 0, 3, 4, 6, 7","Logical order, not photo positions","Check each electrode separately"],MUTED)
c.line([(434,835),(525,835),(580,835)],MUTED);c.circle(580,835,5,MUTED,MUTED)
c.box(1110,1050,440,172,"Strum pads N, O",["Below strings","Left Tempo / O / E9: tap BPM","Right Meter / N / E8: hold select"],PURPLE)
c.line([(1110,1160),(1065,1160),(1008,1155)],PURPLE);c.circle(1008,1155,5,PURPLE,PURPLE)
c.text(44,1300,"Side interfaces: USB-C (charging / USB MIDI), 3.5mm audio, power button and strap slots.",25)
c.text(44,1342,"Functional diagram using one consistent upright view. Dimensions are not for manufacturing.",22,MUTED)
c.save('controls-front')

c=Canvas(1600,1080,"03  Slider Electrodes, Left/Right Positions and Gestures","Upright view. Each group contains two discrete electrodes, not a continuous position sensor.")
c.box(60,160,480,210,"Pitch / Drum Pad",["Below speaker, above strings","Manual labels I/J","I2C SDA GPIO11 / SCL GPIO12","Horizontal Top/Up = upright right"],BLUE)
c.rect(620,165,390,190,"#e8f4ff",BLUE);c.rect(1015,165,390,190,"#e8f4ff",BLUE)
c.text(655,192,"Left: Down / J / E2",31,BLUE);c.text(655,248,"MPR121 electrode E2",23);c.text(655,292,"Drum: Kick / MIDI Note 36",23)
c.text(1050,192,"Right: Up / I / E5",31,BLUE);c.text(1050,248,"MPR121 electrode E5",23);c.text(1050,292,"Drum: Snare / MIDI Note 38",23)
c.line([(680,410),(1350,410)],BLUE,4,True);c.text(770,425,"Left -> right, E2 -> E5: +8191",26,BLUE)
c.line([(1350,495),(680,495)],BLUE,4,True);c.text(770,509,"Right -> left, E5 -> E2: -8191",26,BLUE)
c.text(620,557,"Sequence: 700 ms; bend/return: 500 ms; both released: center 0",23,MUTED)
c.box(60,650,480,218,"Strum / Rhythm",["Below strings; labels N/O","Left: Tempo; right: Meter/pattern","Both read through MPR121","Controls may be redefined in App0"],PURPLE)
c.rect(620,650,390,182,"#f0ebff",PURPLE);c.rect(1015,650,390,182,"#f0ebff",PURPLE)
c.text(655,672,"Left: Tempo / O / E9",31,PURPLE);c.text(655,725,"MPR121 electrode E9",23);c.text(655,770,"At least 3 valid taps: BPM",23)
c.text(1050,672,"Right: Meter / N / E8",31,PURPLE);c.text(1050,725,"MPR121 electrode E8",23);c.text(1050,770,"Hold 2 s: select pattern",23)
c.line([(690,885),(1340,885)],PURPLE,4,True);c.line([(1340,936),(690,936)],PURPLE,4,True)
c.text(730,965,"Either direction can form a Strum gesture; sequence window 700 ms",23,PURPLE)
c.save('sliders-electrodes')

c=Canvas(1600,1130,"02  MCU and Peripheral Buses","Arrows show signal direction, not PCB routing. MCU logic is 3.3 V; all circuits share ground.")
c.box(610,180,370,720,"ESP32-S3",["Dual core, 240 MHz","Flash layout: 16 MiB","PSRAM configuration: OPI","","I2C SDA11 / SCL12","","SPI 13 / 14 / 10","DC18 / RST21 / BL9","","I2S BCLK40 / DOUT41","WS42; no MCLK","","ADC: 1 / 5 / 6 / 8","Digital in: 2 / 4 / 7 / 47","Encoder: 38 / 39","LED48; USB19 / 20"],INK)
c.box(60,170,435,240,"MPR121",["I2C: 0x5A-0x5D; 400 kHz","Strings: E1/E0/E3/E4/E6/E7","Pitch: left E2 / right E5","Strum: left E9 / right E8","IRQ unused; E10/E11 reserved"],BLUE)
c.line([(495,289),(610,289)],BLUE,4,True)
c.box(60,485,435,240,"Battery / charge / power",["BAT+ divider -> GPIO1","CHRG -> GPIO2","STDBY -> GPIO4","3.3 V power state -> GPIO8","GPIO8 is input, not power latch"],GREEN)
c.line([(495,601),(610,601)],GREEN,4,True)
c.box(60,802,435,170,"Joystick / encoder",["Joystick: X6 / Y5 / SW7","Encoder: A38 / B39 / SW47","Switches to GND; internal pullups"],MUTED)
c.line([(495,880),(610,880)],MUTED,4,True)
c.box(1105,170,435,240,"ST7735S TFT",["SCLK13 -> SCLK","MOSI14 -> SDA/MOSI","CS10 / DC18 / RESET21","BL9 -> backlight control","160 x 80; write-only SPI"],BLUE)
c.line([(980,290),(1105,290)],BLUE,4,True)
c.box(1105,478,435,240,"PCM5102-family DAC",["GPIO40 -> BCLK","GPIO41 -> DIN","GPIO42 -> LCK/LRCLK","44100 Hz; 16-bit stereo","Analog output -> audio circuit"],PURPLE)
c.line([(980,580),(1105,580)],PURPLE,4,True)
c.box(1105,780,435,210,"LED ring / native USB",["GPIO48 -> 16 LEDs, GRB","RMT DMA, async output","GPIO19 D- / GPIO20 D+","TinyUSB MIDI device"],GREEN)
c.line([(980,871),(1105,871)],GREEN,4,True)
c.text(60,1034,"Touch electrodes use MPR121, not ESP32 touchRead(). Electrode and GPIO numbers are separate.",25)
c.save('hardware-buses')

c=Canvas(1600,1060,"05  Flash Layout and Update Boundaries","Only App0 firmware and SF2 resources are user upload targets. Other system images remain manufacturer-managed.")
items=[("0x000000","Bootloader","Keep intact; encoder LOW 500 ms selects App1",MUTED),
 ("0x008000","Partition table","Fixed layout; CSV is for App0 compilation only",MUTED),
 ("0x009000","NVS / 20 KiB","Runtime data; size 0x5000",MUTED),
 ("0x00E000","OTA data / 8 KiB","Boot-selection records; managed by OTA APIs",MUTED),
 ("0x010000","App0 / 2 MiB [update]","Wi-Fi application target; BIN <= 0x200000",BLUE),
 ("0x210000","App1 / 2 MiB [keep]","Preinstalled Wi-Fi uploader; use, do not replace",PURPLE),
 ("0x410000","LittleFS / 10.9375 MiB","Resources/settings; SF2 replaceable, no FS image upload",GREEN),
 ("0xF00000","Coredump / 64 KiB","0x10000; 0xF10000-0xFFFFFF unallocated",MUTED)]
for i,(addr,title,body,color) in enumerate(items):
    y=150+i*95;c.text(60,y+19,addr,25,color);c.rect(245,y,1280,80,"#f3f7fb",color)
    c.text(265,y+10,title,26,color);c.text(690,y+20,body,22)
c.text(60,940,"App0: Wi-Fi BIN -> LittleFS staging -> App0 write/validation -> select App0 -> restart",25)
c.text(60,985,"SF2: Wi-Fi SF2 -> replace sound resources -> restart; application and SF2 budgets are separate.",23,MUTED)
c.save('flash-layout')

c=Canvas(1600,940,"04  Cold-Start Recovery and Soft-Power State","Hold the encoder before a true cold start. Charging standby and soft-power release are not the same boot stage.")
c.box(50,180,315,130,"True cold start/reset",["Preinstalled bootloader","Samples GPIO47"],BLUE)
c.box(510,155,490,170,"GPIO47 LOW for 500 ms",["Select existing App1 once","Entry does not rewrite boot record","App1 and partition table must be valid"],PURPLE)
c.line([(365,230),(510,230)],BLUE,4,True)
c.box(1150,155,400,170,"App1 READY",["Wi-Fi Fingersonic; /upload","BIN -> App0; SF2 -> resources","Return to App0 after update"],PURPLE)
c.line([(1000,230),(1150,230)],PURPLE,4,True)
c.box(510,385,490,150,"Encoder not held: normal boot",["Follow stored application selection","App0 shown as the example below"],BLUE)
c.line([(207,310),(207,455),(510,455)],BLUE,4,True)
c.box(50,655,390,172,"App0 standby",["Filesystem, display, battery","GPIO8 millivolts every 100 ms","Default logo: 5 s, then battery"],GREEN)
c.line([(755,535),(755,596),(240,596),(240,655)],BLUE,4,True)
c.box(585,655,450,172,"Initialize full runtime",["GPIO8 >=2600 mV for 100 ms","Start audio/touch/MIDI/LED","Ignore brief startup-load dips"],GREEN)
c.line([(440,734),(585,734)],GREEN,4,True)
c.box(1170,655,380,172,"Running shutdown",["GPIO8 <=2600 mV for 200 ms","Sound off, black LEDs, restart","Restart releases runtime tasks"],GREEN)
c.line([(1035,734),(1170,734)],GREEN,4,True)
c.text(50,870,"Share ADC locking with battery/joystick. Keep power waits and file writes out of the audio task.",24)
c.save('boot-power-flow')

c=Canvas(1600,980,"06  Recovery SF2 Presets and Performance Parts","Fingersonic_Gtr_H.sf2 contains 7 presets. BIN updates App0; SF2 replaces sounds. Both upload through App1 Wi-Fi.")
c.box(60,150,550,425,"Included recovery bank",["Bank 0 / Program 0: Steel Gtr","Bank 0 / Program 1: Funk Gtr","Bank 0 / Program 2: Jazz Gtr","Bank 0 / Program 3: Mute Gtr","Bank 0 / Program 4: Overd. Gtr","","Bank 128 / Program 0: Standard Kit","Bank 128 / Program 1: Piano"],BLUE)
c.box(810,150,720,172,"Live and accompaniment guitars",["Live MIDI Ch 1; automatic guitar Ch 2","Bank 0, currently selected Program 0-4","Independent note releases, same guitar timbre"],BLUE)
c.line([(610,222),(810,222)],BLUE,4,True)
c.box(810,350,720,172,"Drum part",["MIDI Ch 10 -> Bank 128 / Program 0","Note 36 Kick; Note 38 Snare; Note 42 Hi-Hat","Preserve these keys for patterns/drum pads"],PURPLE)
c.line([(610,444),(730,444),(730,426),(810,426)],PURPLE,4,True)
c.box(810,550,720,172,"Piano and lead/melody roles",["MIDI Ch 11 -> Bank 128 / Program 1: Piano","No separate Lead preset in this supplied bank","A lead role uses an application-selected instrument"],GREEN)
c.line([(610,493),(710,493),(710,625),(810,625)],GREEN,4,True)
c.box(60,750,1470,165,"User-replaceable sound resources",["One SF2 contains multiple presets. Refresh the program catalog when changing to another bank.","Keep Bank 0, Bank 128/P0 drums and Bank 128/P1 melody for default-app compatibility.","Recovery restores sound resources only; restoring default App0 requires its separate application BIN."],MUTED)
c.save('sf2-voices')
print(f"Generated 6 English SVG and 6 English PNG diagrams in {OUT}")
