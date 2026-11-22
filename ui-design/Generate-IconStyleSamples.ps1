Add-Type -AssemblyName System.Drawing

$outDir = $PSScriptRoot
$scale = 2
$variants = @(
    @{ Id = 'D'; Title = '阿里多彩扁平插画'; Style = 'flat'; Note = '明亮色块、天气与生活物件图解' },
    @{ Id = 'E'; Title = '软胶 3D 拟物'; Style = '3d'; Note = '渐变高光、圆润立体小物件' },
    @{ Id = 'F'; Title = '手绘贴纸风'; Style = 'sticker'; Note = '粗描边、白色贴纸边与彩色图案' }
)

function ColorOf([string]$hex) { [System.Drawing.ColorTranslator]::FromHtml($hex) }
function MakePen([string]$hex,[single]$width) {
    $p = [System.Drawing.Pen]::new((ColorOf $hex),$width)
    $p.StartCap = 'Round'; $p.EndCap = 'Round'; $p.LineJoin = 'Round'
    return $p
}
function MakeBrush([string]$hex) { [System.Drawing.SolidBrush]::new((ColorOf $hex)) }
function RoundPath([single]$x,[single]$y,[single]$w,[single]$h,[single]$r) {
    $p = [System.Drawing.Drawing2D.GraphicsPath]::new(); $d = 2*$r
    $p.AddArc($x,$y,$d,$d,180,90); $p.AddArc($x+$w-$d,$y,$d,$d,270,90)
    $p.AddArc($x+$w-$d,$y+$h-$d,$d,$d,0,90); $p.AddArc($x,$y+$h-$d,$d,$d,90,90)
    $p.CloseFigure(); return $p
}
function FillRound($g,[single]$x,[single]$y,[single]$w,[single]$h,[single]$r,[string]$color) {
    $b=MakeBrush $color; $p=RoundPath $x $y $w $h $r; $g.FillPath($b,$p); $p.Dispose(); $b.Dispose()
}
function DrawText($g,[string]$s,[single]$x,[single]$y,[single]$w,[single]$h,[single]$size,[string]$color,[bool]$bold=$false,[bool]$center=$false) {
    $style=if($bold){[System.Drawing.FontStyle]::Bold}else{[System.Drawing.FontStyle]::Regular}
    $f=[System.Drawing.Font]::new('Microsoft YaHei UI',$size,$style,[System.Drawing.GraphicsUnit]::Pixel)
    $b=MakeBrush $color
    if($center){$sf=[System.Drawing.StringFormat]::new();$sf.Alignment='Center';$sf.LineAlignment='Center';$g.DrawString($s,$f,$b,[System.Drawing.RectangleF]::new($x,$y,$w,$h),$sf);$sf.Dispose()}
    else{$g.DrawString($s,$f,$b,$x,$y)}
    $f.Dispose();$b.Dispose()
}
function CloudPath([single]$x,[single]$y,[single]$w,[single]$h) {
    $p=[System.Drawing.Drawing2D.GraphicsPath]::new()
    $p.AddBezier($x+.31*$w,$y+.82*$h,$x+.14*$w,$y+.82*$h,$x+.06*$w,$y+.69*$h,$x+.06*$w,$y+.54*$h)
    $p.AddBezier($x+.06*$w,$y+.54*$h,$x+.06*$w,$y+.37*$h,$x+.21*$w,$y+.25*$h,$x+.40*$w,$y+.25*$h)
    $p.AddBezier($x+.40*$w,$y+.25*$h,$x+.48*$w,$y+.02*$h,$x+.81*$w,$y+.05*$h,$x+.86*$w,$y+.35*$h)
    $p.AddBezier($x+.86*$w,$y+.35*$h,$x+1.05*$w,$y+.34*$h,$x+1.08*$w,$y+.64*$h,$x+.91*$w,$y+.74*$h)
    $p.AddBezier($x+.91*$w,$y+.74*$h,$x+.88*$w,$y+.82*$h,$x+.80*$w,$y+.84*$h,$x+.70*$w,$y+.84*$h)
    $p.AddLine($x+.70*$w,$y+.84*$h,$x+.31*$w,$y+.82*$h); $p.CloseFigure(); return $p
}
function DrawSun($g,[single]$cx,[single]$cy,[single]$r,[string]$style) {
    $gold='#E9C95A'; $p=MakePen $gold 2.0; $b=MakeBrush $gold
    if($style -eq 'line'){$g.DrawEllipse($p,$cx-$r,$cy-$r,2*$r,2*$r)}
    elseif($style -eq '3d'){$grad=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.RectangleF]::new($cx-$r,$cy-$r,2*$r,2*$r),(ColorOf '#FFF4A8'),(ColorOf '#E98B3C'),35);$g.FillEllipse($grad,$cx-$r,$cy-$r,2*$r,2*$r);$grad.Dispose();$shine=MakeBrush '#FFF8D4';$g.FillEllipse($shine,$cx-$r*.48,$cy-$r*.55,$r*.72,$r*.56);$shine.Dispose()}
    else{$g.FillEllipse($b,$cx-$r,$cy-$r,2*$r,2*$r);if($style -eq 'sticker'){$white=MakePen '#F7F0D7' 3.8;$g.DrawEllipse($white,$cx-$r,$cy-$r,2*$r,2*$r);$white.Dispose();$g.FillEllipse($b,$cx-$r+1,$cy-$r+1,2*$r-2,2*$r-2)}}
    for($i=0;$i -lt 8;$i++){
        $a=[Math]::PI*$i/4; $r1=$r+2; $r2=$r+5
        $g.DrawLine($p,$cx+[Math]::Cos($a)*$r1,$cy+[Math]::Sin($a)*$r1,$cx+[Math]::Cos($a)*$r2,$cy+[Math]::Sin($a)*$r2)
    }
    $p.Dispose();$b.Dispose()
}
function DrawCloud($g,[single]$x,[single]$y,[single]$w,[single]$h,[string]$style,[string]$color='#B8D47A') {
    $path=CloudPath $x $y $w $h; $p=MakePen $color 1.8; $b=MakeBrush $color
    if($style -eq 'line'){$g.DrawPath($p,$path)}
    elseif($style -eq '3d'){$grad=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.RectangleF]::new($x,$y,$w,$h),(ColorOf '#FFFFFF'),(ColorOf '#9EAD9A'),90);$g.FillPath($grad,$path);$grad.Dispose();$rim=MakePen '#82927F' 1;$g.DrawPath($rim,$path);$rim.Dispose();$hi=MakeBrush '#FFFFFF';$g.FillEllipse($hi,$x+.30*$w,$y+.28*$h,.28*$w,.10*$h);$hi.Dispose()}
    elseif($style -eq 'sticker'){$white=MakePen '#FFF8E5' 5;$g.DrawPath($white,$path);$g.FillPath($b,$path);$outline=MakePen '#4F663C' 1.1;$g.DrawPath($outline,$path);$white.Dispose();$outline.Dispose();$hi=MakeBrush '#E9EAA3';$g.FillEllipse($hi,$x+.30*$w,$y+.28*$h,.26*$w,.10*$h);$hi.Dispose()}
    else{$g.FillPath($b,$path);$hi=MakeBrush '#F2EBCB';$g.FillEllipse($hi,$x+.30*$w,$y+.28*$h,.28*$w,.10*$h);$hi.Dispose()}
    $p.Dispose();$b.Dispose();$path.Dispose()
}
function DrawWeather($g,[single]$x,[single]$y,[single]$w,[single]$h,[string]$kind,[string]$style) {
    if($kind -eq 'sunny') { DrawSun $g ($x+$w*.53) ($y+$h*.47) ($w*.22) $style; return }
    if($kind -eq 'partly') { DrawSun $g ($x+$w*.32) ($y+$h*.34) ($w*.20) $style; DrawCloud $g ($x+$w*.19) ($y+$h*.27) ($w*.76) ($h*.68) $style; return }
    if($kind -eq 'rain') {
        DrawCloud $g ($x+$w*.04) ($y+$h*.08) ($w*.88) ($h*.69) $style
        $p=MakePen '#E9C95A' 1.8
        foreach($rx in @(.30,.52,.74)){$g.DrawLine($p,$x+$w*$rx,$y+$h*.78,$x+$w*($rx-.06),$y+$h*.96)}
        $p.Dispose(); return
    }
    DrawCloud $g ($x+$w*.08) ($y+$h*.10) ($w*.84) ($h*.75) $style
}
function LifePen($style,[string]$color='#B8D47A') { $width=if($style -eq 'line'){2.1}else{1.6}; return (MakePen $color $width) }
function DrawLife($g,[single]$cx,[single]$cy,[string]$kind,[string]$style) {
    $green='#B8D47A';$gold='#E9C95A';$dark='#355139';$p=LifePen $style; $b=MakeBrush $green
    if($kind -eq 'shirt') {
        $x=$cx-11;$y=$cy-10
        # Broaden sleeves and torso so the shirt reads clearly at 24 px.
        $q=[System.Drawing.Drawing2D.GraphicsPath]::new();$q.AddLine($x+17,$y+2,$x+14,$y+1);$q.AddBezier($x+14,$y+1,$x+13.5,$y+5,$x+8.5,$y+5,$x+8,$y+1);$q.AddLine($x+8,$y+1,$x+5,$y+2);$q.AddLine($x+5,$y+2,$x,$y+8);$q.AddLine($x,$y+8,$x+5,$y+11);$q.AddLine($x+5,$y+11,$x+7,$y+8);$q.AddLine($x+7,$y+8,$x+7,$y+20);$q.AddLine($x+7,$y+20,$x+15,$y+20);$q.AddLine($x+15,$y+20,$x+15,$y+8);$q.AddLine($x+15,$y+8,$x+17,$y+11);$q.AddLine($x+17,$y+11,$x+22,$y+8);$q.CloseFigure()
        if($style -eq 'line'){$g.DrawPath($p,$q)}else{
            if($style -eq '3d'){$grad=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.RectangleF]::new($x,$y,22,22),(ColorOf '#EAF6B2'),(ColorOf '#719A56'),35);$g.FillPath($grad,$q);$grad.Dispose()}
            else{if($style -eq 'sticker'){$white=MakePen '#FFF8E5' 4;$g.DrawPath($white,$q);$white.Dispose()};$g.FillPath($b,$q);if($style -eq 'sticker'){$outline=MakePen '#4F663C' 1.1;$g.DrawPath($outline,$q);$outline.Dispose()}}
            $detailColor=if($style -eq 'flat'){'#E9C95A'}else{$gold};$detail=MakePen $detailColor 1.7
            $g.DrawLine($detail,$x+9,$y+3,$x+11,$y+4)
            if($style -eq 'duotone'){$accent=MakeBrush '#E9C95A';$g.FillEllipse($accent,$x+9,$y+11,4,3);$accent.Dispose()}
            $detail.Dispose()
        };$q.Dispose()
    } elseif($kind -eq 'dumbbell') {
        if($style -eq 'line'){$g.DrawLine($p,$cx-6,$cy+6,$cx+6,$cy-6);$g.DrawLine($p,$cx-8,$cy+4,$cx-4,$cy+8);$g.DrawLine($p,$cx-10,$cy+2,$cx-6,$cy+6);$g.DrawLine($p,$cx+4,$cy-8,$cx+8,$cy-4);$g.DrawLine($p,$cx+6,$cy-10,$cx+10,$cy-6)}else{
            $bar=MakePen $green 4;$bar.StartCap='Round';$bar.EndCap='Round';$g.DrawLine($bar,$cx-6,$cy+6,$cx+6,$cy-6);$bar.Dispose()
            $plate=MakePen $gold 3;$plate.StartCap='Round';$plate.EndCap='Round';$g.DrawLine($plate,$cx-9,$cy+3,$cx-5,$cy+7);$g.DrawLine($plate,$cx+5,$cy-7,$cx+9,$cy-3);$plate.Dispose()
            if($style -eq 'duotone'){$grip=MakePen '#F2EBCB' 1.3;$g.DrawLine($grip,$cx-4,$cy+4,$cx+4,$cy-4);$grip.Dispose()}
        }
    } elseif($kind -eq 'pill') {
        $state=$g.Save();$g.TranslateTransform($cx,$cy);$g.RotateTransform(-45);$rp=RoundPath -6 -9 12 18 6
        if($style -eq 'line'){$g.DrawPath($p,$rp);$g.DrawLine($p,-6,0,6,0)}else{
            if($style -eq '3d'){$grad=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.RectangleF]::new(-6,-9,12,18),(ColorOf '#FFF3A1'),(ColorOf '#E98358'),35);$g.FillPath($grad,$rp);$grad.Dispose()}
            else{if($style -eq 'sticker'){$white=MakePen '#FFF8E5' 4;$g.DrawPath($white,$rp);$white.Dispose()};$g.FillPath($b,$rp);if($style -eq 'sticker'){$outline=MakePen '#4F663C' 1;$g.DrawPath($outline,$rp);$outline.Dispose()}}
            $seam=MakePen $gold 2.0;$g.DrawLine($seam,-6,0,6,0);$seam.Dispose()
        }
        $rp.Dispose();$g.Restore($state)
    } else {
        $car=RoundPath ($cx-12) ($cy-1) 24 12 3
        if($style -eq 'line'){$g.DrawPath($p,$car);$g.DrawLine($p,$cx-7,$cy+1,$cx+7,$cy+1)}else{
            if($style -eq '3d'){$grad=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.RectangleF]::new($cx-12,$cy-1,24,12),(ColorOf '#F4DC74'),(ColorOf '#60A889'),35);$g.FillPath($grad,$car);$grad.Dispose()}
            else{if($style -eq 'sticker'){$white=MakePen '#FFF8E5' 4;$g.DrawPath($white,$car);$white.Dispose()};$g.FillPath($b,$car);if($style -eq 'sticker'){$outline=MakePen '#4F663C' 1;$g.DrawPath($outline,$car);$outline.Dispose()}}
            $glass=MakeBrush '#F2EBCB';$g.FillRectangle($glass,$cx-6,$cy+1,12,3);$glass.Dispose()
        }
        $car.Dispose();$g.DrawLine($p,$cx-7,$cy+11,$cx-7,$cy+13);$g.DrawLine($p,$cx+7,$cy+11,$cx+7,$cy+13);$g.DrawEllipse($p,$cx-7,$cy+4,2,2);$g.DrawEllipse($p,$cx+5,$cy+4,2,2)
    }
    $p.Dispose();$b.Dispose()
}
function DrawScreen([string]$style) {
    $bmp=[System.Drawing.Bitmap]::new(320,240,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g=[System.Drawing.Graphics]::FromImage($bmp);$g.SmoothingMode='AntiAlias';$g.PixelOffsetMode='HighQuality';$g.TextRenderingHint='AntiAliasGridFit'
    $bg=[System.Drawing.Drawing2D.LinearGradientBrush]::new([System.Drawing.Rectangle]::new(0,0,320,240),(ColorOf '#162519'),(ColorOf '#20351F'),35);$g.FillRectangle($bg,0,0,320,240);$bg.Dispose()
    $card=MakeBrush '#2C442D';$edge=MakePen '#687849' 1
    foreach($r in @(@(8,8,304,124,16),@(8,140,304,92,15))){$rp=RoundPath $r[0] $r[1] $r[2] $r[3] $r[4];$g.FillPath($card,$rp);$g.DrawPath($edge,$rp);$rp.Dispose()}
    $card.Dispose();$edge.Dispose()
    $div=MakePen '#536640' 1;$g.DrawLine($div,184,22,184,96);$g.DrawLine($div,20,166,300,166);foreach($xx in @(90,162,234)){$g.DrawLine($div,$xx,174,$xx,225)};$div.Dispose()
    FillRound $g 16 102 164 24 12 '#3B4D2E';$dot=MakeBrush '#E9C95A';$g.FillEllipse($dot,25,111,6,6);$dot.Dispose()
    DrawText $g '北京' 20 13 100 22 16 '#F2EBCB' $true
    DrawText $g '5°' 17 31 94 65 48 '#F5F0D8'
    DrawWeather $g 104 34 48 44 'partly' $style;DrawText $g '多云' 100 79 56 22 16 '#E8E8C9' $false $true
    DrawText $g '明天' 190 19 56 20 16 '#C2C9A2' $false $true;DrawWeather $g 204 40 28 28 'rain' $style;DrawText $g '小雨' 192 69 56 20 16 '#E4E8C8' $false $true
    DrawText $g '后天' 254 19 56 20 16 '#C2C9A2' $false $true;DrawWeather $g 268 40 28 28 'sunny' $style;DrawText $g '晴' 256 69 56 20 16 '#E4E8C8' $false $true
    DrawText $g '更新：18分钟前' 38 103 142 23 16 '#F2E6A7'
    DrawText $g '生活指数' 20 143 100 22 16 '#F2EBCB' $true;DrawText $g '今天' 264 144 36 20 16 '#BAC294' $false $true
    $kinds=@('shirt','dumbbell','pill','car');$labels=@('穿衣','运动','感冒','洗车');$values=@('较冷','较不宜','少发','不宜');$cxs=@(55,126,198,270)
    for($i=0;$i -lt 4;$i++){DrawLife $g $cxs[$i] 178 $kinds[$i] $style;DrawText $g $labels[$i] ($cxs[$i]-32) 188 64 19 16 '#C2C9A2' $false $true;DrawText $g $values[$i] ($cxs[$i]-32) 207 64 21 16 '#F2EBCB' $true $true}
    $g.Dispose();return $bmp
}

$board=[System.Drawing.Bitmap]::new(1920,560,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$bg=MakeBrush '#111D15';$boardG=[System.Drawing.Graphics]::FromImage($board);$boardG.SmoothingMode='AntiAlias';$boardG.TextRenderingHint='AntiAliasGridFit';$boardG.FillRectangle($bg,0,0,1920,560);$bg.Dispose()
$titles=MakeBrush '#F2EBCB';$muted=MakeBrush '#B8C49A';$headerFont=[System.Drawing.Font]::new('Microsoft YaHei UI',22,[System.Drawing.FontStyle]::Bold,[System.Drawing.GraphicsUnit]::Pixel);$noteFont=[System.Drawing.Font]::new('Microsoft YaHei UI',15,[System.Drawing.FontStyle]::Regular,[System.Drawing.GraphicsUnit]::Pixel)
for($i=0;$i -lt $variants.Count;$i++){
    $v=$variants[$i];$x=$i*640
    $boardG.DrawString("方案 $($v.Id)  ·  $($v.Title)",$headerFont,$titles,$x+28,8)
    $boardG.DrawString($v.Note,$noteFont,$muted,$x+30,38)
    $screen=DrawScreen $v.Style;$dest=[System.Drawing.Rectangle]::new($x,72,640,480);$boardG.InterpolationMode='NearestNeighbor';$boardG.PixelOffsetMode='Half';$boardG.DrawImage($screen,$dest);$screen.Dispose()
}
$headerFont.Dispose();$noteFont.Dispose();$titles.Dispose();$muted.Dispose();$boardG.Dispose()
$board.Save((Join-Path $outDir 'weather-icon-style-samples.png'),[System.Drawing.Imaging.ImageFormat]::Png);$board.Dispose()

for($i=0;$i -lt $variants.Count;$i++){
    $v=$variants[$i];$screen=DrawScreen $v.Style
    $large=[System.Drawing.Bitmap]::new(960,720,[System.Drawing.Imaging.PixelFormat]::Format32bppArgb);$g=[System.Drawing.Graphics]::FromImage($large);$g.InterpolationMode='NearestNeighbor';$g.PixelOffsetMode='Half';$g.DrawImage($screen,[System.Drawing.Rectangle]::new(0,0,960,720));$g.Dispose();$screen.Dispose()
    $large.Save((Join-Path $outDir "weather-icon-style-$($v.Id).png"),[System.Drawing.Imaging.ImageFormat]::Png);$large.Dispose()
}
Write-Output "Comparison: $(Join-Path $outDir 'weather-icon-style-samples.png')"
