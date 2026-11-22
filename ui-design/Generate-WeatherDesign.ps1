Add-Type -AssemblyName System.Drawing
$root = Split-Path -Parent $PSScriptRoot
$nl = [Environment]::NewLine
$bg = [System.Drawing.Bitmap]::new(320, 240, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bg)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

function Brush([string]$hex) { [System.Drawing.SolidBrush]::new([System.Drawing.ColorTranslator]::FromHtml($hex)) }
function RoundPath([single]$x,[single]$y,[single]$w,[single]$h,[single]$r) {
    $p=[System.Drawing.Drawing2D.GraphicsPath]::new(); $d=2*$r
    $p.AddArc($x,$y,$d,$d,180,90); $p.AddArc($x+$w-$d,$y,$d,$d,270,90)
    $p.AddArc($x+$w-$d,$y+$h-$d,$d,$d,0,90); $p.AddArc($x,$y+$h-$d,$d,$d,90,90); $p.CloseFigure(); $p
}
function FillRound($gr,[single]$x,[single]$y,[single]$w,[single]$h,[single]$r,$b) {
    $p=RoundPath $x $y $w $h $r; $gr.FillPath($b,$p); $p.Dispose()
}
function DrawRound($gr,[single]$x,[single]$y,[single]$w,[single]$h,[single]$r,$pen) {
    $p=RoundPath $x $y $w $h $r; $gr.DrawPath($pen,$p); $p.Dispose()
}
function Text($gr,[string]$s,[single]$x,[single]$y,[single]$w,[single]$h,[single]$size,[string]$color,[bool]$bold=$false,[bool]$center=$false) {
    $style=if($bold){[System.Drawing.FontStyle]::Bold}else{[System.Drawing.FontStyle]::Regular}
    $f=[System.Drawing.Font]::new('Microsoft YaHei UI',$size,$style,[System.Drawing.GraphicsUnit]::Pixel); $b=Brush $color
    if($center){$sf=[System.Drawing.StringFormat]::new();$sf.Alignment='Center';$sf.LineAlignment='Center';$gr.DrawString($s,$f,$b,[System.Drawing.RectangleF]::new($x,$y,$w,$h),$sf);$sf.Dispose()}
    else{$gr.DrawString($s,$f,$b,$x,$y)}
    $f.Dispose();$b.Dispose()
}
function Cloud($gr,[single]$x,[single]$y,[single]$w,[single]$h,[string]$color) {
    # Lucide cloud silhouette, redrawn as a fine, rounded outline for the small display.
    $p=[System.Drawing.Drawing2D.GraphicsPath]::new()
    $p.AddBezier($x+.34*$w,$y+.82*$h,$x+.18*$w,$y+.82*$h,$x+.08*$w,$y+.70*$h,$x+.08*$w,$y+.55*$h)
    $p.AddBezier($x+.08*$w,$y+.55*$h,$x+.08*$w,$y+.37*$h,$x+.22*$w,$y+.24*$h,$x+.42*$w,$y+.24*$h)
    $p.AddBezier($x+.42*$w,$y+.24*$h,$x+.50*$w,$y+.02*$h,$x+.82*$w,$y+.06*$h,$x+.87*$w,$y+.36*$h)
    $p.AddBezier($x+.87*$w,$y+.36*$h,$x+1.06*$w,$y+.34*$h,$x+1.08*$w,$y+.64*$h,$x+.91*$w,$y+.74*$h)
    $p.AddBezier($x+.91*$w,$y+.74*$h,$x+.88*$w,$y+.82*$h,$x+.81*$w,$y+.84*$h,$x+.72*$w,$y+.84*$h)
    $p.AddLine($x+.72*$w,$y+.84*$h,$x+.34*$w,$y+.84*$h)
    $pen=[System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml($color),1.7)
    $pen.StartCap='Round';$pen.EndCap='Round';$pen.LineJoin='Round';$gr.DrawPath($pen,$p)
    $pen.Dispose();$p.Dispose()
}

# Static screen art: a forest-green canvas, moss cards, warm accent colors and four line icons.
$b=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.Rectangle]::new(0,0,320,240),[System.Drawing.Color]::FromArgb(22,37,25),[System.Drawing.Color]::FromArgb(32,53,31),35)
$g.FillRectangle($b,0,0,320,240);$b.Dispose()
$pb=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.Rectangle]::new(8,8,304,124),[System.Drawing.Color]::FromArgb(48,69,44),[System.Drawing.Color]::FromArgb(41,59,37),35)
$edge=[System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(84,104,58),1)
FillRound $g 8 8 304 124 16 $pb;DrawRound $g 8.5 8.5 303 123 16 $edge
FillRound $g 8 140 304 92 15 $pb;DrawRound $g 8.5 140.5 303 91 15 $edge
$pb.Dispose();$edge.Dispose()
$line=[System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(76,94,51),1)
$g.DrawLine($line,184,22,184,96)
$g.DrawLine($line,90,174,90,225);$g.DrawLine($line,162,174,162,225);$g.DrawLine($line,234,174,234,225);$line.Dispose()
$pill=Brush '#3B4D2E';FillRound $g 16 102 164 24 12 $pill;$pill.Dispose()
$dot=Brush '#E7C95A';$g.FillEllipse($dot,25,111,6,6);$dot.Dispose()
# F-style sticker icons: draw a broad cream sticker edge, then the colored inner stroke.
$sticker=[System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml('#F2EBCB'),3.2)
$gold=[System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml('#E7C95A'),2.2)
$green=[System.Drawing.Pen]::new([System.Drawing.ColorTranslator]::FromHtml('#B8D47A'),2.2)
foreach($pen in @($sticker,$gold,$green)){$pen.StartCap='Round';$pen.EndCap='Round';$pen.LineJoin='Round'}
# Shirt: wider torso and fuller sleeves so it reads as clothing at the panel's small size.
$sp=[System.Drawing.Drawing2D.GraphicsPath]::new()
$sp.AddLine(65,168.8,61.3,167.3);$sp.AddBezier(61.3,167.3,61,171.4,57.5,173.2,53,173.2);$sp.AddBezier(53,173.2,48.5,173.2,45,171.4,44.7,167.3)
$sp.AddLine(44.7,167.3,41,168.8);$sp.AddLine(41,168.8,36.2,178.7);$sp.AddLine(36.2,178.7,42.5,181.9);$sp.AddLine(42.5,181.9,45.1,179.1)
$sp.AddLine(45.1,179.1,45.1,189);$sp.AddLine(45.1,189,60.9,189);$sp.AddLine(60.9,189,60.9,179.1);$sp.AddLine(60.9,179.1,63.5,181.9);$sp.AddLine(63.5,181.9,69.8,178.7);$sp.AddLine(69.8,178.7,65,168.8);$sp.CloseFigure();$g.DrawPath($sticker,$sp);$g.DrawPath($gold,$sp);$sp.Dispose()
# Dumbbell: two weight blocks joined by a diagonal grip.
$g.DrawLine($sticker,116,183,132,167);$g.DrawLine($sticker,113.5,180.5,118.5,185.5);$g.DrawLine($sticker,116,178,121,183)
$g.DrawLine($sticker,127,173,132,178);$g.DrawLine($sticker,129.5,165.5,134.5,170.5);$g.DrawLine($sticker,132,163,137,168)
$g.DrawLine($green,116,183,132,167);$g.DrawLine($green,113.5,180.5,118.5,185.5);$g.DrawLine($green,116,178,121,183)
$g.DrawLine($green,127,173,132,178);$g.DrawLine($green,129.5,165.5,134.5,170.5);$g.DrawLine($green,132,163,137,168)
# Pill: rounded capsule split on its diagonal, following Lucide's pill SVG.
$state=$g.Save();$g.TranslateTransform(198,177);$g.RotateTransform(-45)
$pillPath=RoundPath -5.5 -10 11 20 5.5;$g.DrawPath($sticker,$pillPath);$g.DrawPath($gold,$pillPath);$pillPath.Dispose();$g.DrawLine($sticker,-5.5,0,5.5,0);$g.DrawLine($gold,-5.5,0,5.5,0);$g.Restore($state)
# Car-front: roof/windscreen, rounded cabin outline, front lights and lower bumper.
$cp=[System.Drawing.Drawing2D.GraphicsPath]::new();$cp.AddLine(285,176,283,178);$cp.AddLine(283,178,281.5,171.5);$cp.AddBezier(281.5,171.5,281.1,170.4,280.2,169.7,279,169.7)
$cp.AddLine(279,169.7,264,169.7);$cp.AddBezier(264,169.7,262.8,169.7,261.9,170.4,261.5,171.5);$cp.AddLine(261.5,171.5,260,178);$cp.AddLine(260,178,258,176)
$g.DrawPath($sticker,$cp);$g.DrawPath($green,$cp);$cp.Dispose();$carBody=RoundPath 258 178 27 9 2;$g.DrawPath($sticker,$carBody);$g.DrawPath($green,$carBody);$carBody.Dispose()
$g.DrawLine($sticker,261,187,261,189);$g.DrawLine($sticker,282,187,282,189);$g.DrawLine($sticker,263,182,264,182);$g.DrawLine($sticker,279,182,280,182)
$g.DrawLine($green,261,187,261,189);$g.DrawLine($green,282,187,282,189);$g.DrawLine($green,263,182,264,182);$g.DrawLine($green,279,182,280,182)
$sticker.Dispose();$gold.Dispose();$green.Dispose()

$bg.Save((Join-Path $PSScriptRoot 'weather-v2-background-320.png'),[System.Drawing.Imaging.ImageFormat]::Png)
$background = $bg.Clone()
# Design preview uses realistic sample data over the same background art.
Text $g '北京' 20 13 100 22 16 '#F2EBCB' $true
Text $g '5°' 17 31 94 65 48 '#F5F0D8'
Cloud $g 104 34 48 44 '#B8D47A';Text $g '多云' 100 79 56 22 16 '#E8E8C9' $false $true
Text $g '明天' 190 19 56 20 16 '#C2C9A2' $false $true;Cloud $g 204 40 28 28 '#B8D47A';Text $g '多云' 192 69 56 20 16 '#E4E8C8' $false $true
Text $g '后天' 254 19 56 20 16 '#C2C9A2' $false $true;Cloud $g 268 40 28 28 '#9DB96A';Text $g '阴' 256 69 56 20 16 '#E4E8C8' $false $true
Text $g '更新：18分钟前' 38 103 142 23 16 '#F2E6A7'
Text $g '生活指数' 20 143 100 22 16 '#F2EBCB' $true;Text $g '今天' 264 144 36 20 16 '#BAC294' $false $true
Text $g '穿衣' 23 188 64 19 16 '#C2C9A2' $false $true;Text $g '运动' 95 188 64 19 16 '#C2C9A2' $false $true
Text $g '感冒' 167 188 64 19 16 '#C2C9A2' $false $true;Text $g '洗车' 239 188 64 19 16 '#C2C9A2' $false $true
Text $g '较冷' 23 207 64 21 16 '#F2EBCB' $true $true;Text $g '较不宜' 95 207 64 21 16 '#F2EBCB' $true $true
Text $g '少发' 167 207 64 21 16 '#F2EBCB' $true $true;Text $g '不宜' 239 207 64 21 16 '#F2EBCB' $true $true
$g.Dispose()
$large=[System.Drawing.Bitmap]::new(960,720,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$lg=[System.Drawing.Graphics]::FromImage($large);$lg.InterpolationMode='NearestNeighbor';$lg.PixelOffsetMode='Half'
$lg.DrawImage($bg,[System.Drawing.Rectangle]::new(0,0,960,720));$lg.Dispose()
$large.Save((Join-Path $PSScriptRoot 'weather-v2-design.png'),[System.Drawing.Imaging.ImageFormat]::Png)

# Generate the 320x240 RGB565 background used by LVGL (little-endian).
$rect=[System.Drawing.Rectangle]::new(0,0,320,240)
$bits=$background.LockBits($rect,[System.Drawing.Imaging.ImageLockMode]::ReadOnly,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$raw=[byte[]]::new($bits.Stride*240);[System.Runtime.InteropServices.Marshal]::Copy($bits.Scan0,$raw,0,$raw.Length);$background.UnlockBits($bits)
$sb=[System.Text.StringBuilder]::new(1100000)
$nl=[Environment]::NewLine
[void]$sb.Append('#include "lvgl.h"'+$nl+'#ifndef LV_ATTRIBUTE_MEM_ALIGN'+$nl+'#define LV_ATTRIBUTE_MEM_ALIGN'+$nl+'#endif'+$nl+'#ifndef LV_ATTRIBUTE_LARGE_CONST'+$nl+'#define LV_ATTRIBUTE_LARGE_CONST'+$nl+'#endif'+$nl+'#ifndef LV_ATTRIBUTE_IMG_WEATHER_BG'+$nl+'#define LV_ATTRIBUTE_IMG_WEATHER_BG'+$nl+'#endif'+$nl+$nl)
[void]$sb.Append('#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP != 0'+$nl+'#error "Weather background expects RGB565 with LV_COLOR_16_SWAP=0"'+$nl+'#endif'+$nl+$nl)
[void]$sb.Append('static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_IMG_WEATHER_BG uint8_t weather_bg_map[] = {'+$nl)
$pixel=0
for($y=0;$y -lt 240;$y++){for($x=0;$x -lt 320;$x++){
    $i=$y*$bits.Stride+$x*4;$blue=[int]$raw[$i];$green=[int]$raw[$i+1];$red=[int]$raw[$i+2]
    $v=(($red -band 0xF8) -shl 8) -bor (($green -band 0xFC) -shl 3) -bor ($blue -shr 3)
    [void]$sb.AppendFormat('0x{0:x2}, 0x{1:x2},',$v -band 255,($v -shr 8) -band 255);$pixel++
    if(($pixel%12)-eq 0){[void]$sb.Append($nl)}else{[void]$sb.Append(' ')}
}}
[void]$sb.Append($nl+'};'+$nl+$nl+'const lv_img_dsc_t weather_bg = {'+$nl+'  .header.cf = LV_IMG_CF_TRUE_COLOR,'+$nl+'  .header.always_zero = 0,'+$nl+'  .header.reserved = 0,'+$nl+'  .header.w = 320,'+$nl+'  .header.h = 240,'+$nl+'  .data_size = sizeof(weather_bg_map),'+$nl+'  .data = weather_bg_map,'+$nl+'};'+$nl)
[System.IO.File]::WriteAllText((Join-Path $root 'main/weather/weather_bg.c'),$sb.ToString(),[System.Text.Encoding]::ASCII)
$large.Dispose();$background.Dispose();$bg.Dispose()
Write-Output "Design: $(Join-Path $PSScriptRoot 'weather-v2-design.png')"
Write-Output "Background source regenerated: $(Join-Path $root 'main/weather/weather_bg.c')"
