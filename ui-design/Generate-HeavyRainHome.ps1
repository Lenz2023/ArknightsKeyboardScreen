Add-Type -AssemblyName System.Drawing

$root = Split-Path -Parent $PSScriptRoot
$assetRoot = Join-Path $PSScriptRoot 'heavy-rain-weather'
$characterSource = Join-Path $PSScriptRoot 'heavy-rain\heavy-rain-cutout.png'
$scenes = @(
    @{ Name = 'sunny';  File = 'weather-sunny.png';  Caption = '晴'; Time = '13:46'; TintR = 215; TintG = 191; TintB = 147; TintMix = 0.020; BgR = 139; BgG = 105; BgB = 58; BgAlpha = 74 },
    @{ Name = 'cloudy'; File = 'weather-cloudy.png'; Caption = '阴'; Time = '13:46'; TintR = 177; TintG = 187; TintB = 196; TintMix = 0.025; BgR = 92; BgG = 113; BgB = 138; BgAlpha = 52 },
    @{ Name = 'rain';   File = 'weather-rain.png';   Caption = '雨'; Time = '13:46'; TintR = 128; TintG = 164; TintB = 184; TintMix = 0.035; BgR = 13; BgG = 39; BgB = 77; BgAlpha = 78 },
    @{ Name = 'snow';   File = 'weather-snow.png';   Caption = '雪'; Time = '13:46'; TintR = 207; TintG = 216; TintB = 222; TintMix = 0.025; BgR = 89; BgG = 125; BgB = 157; BgAlpha = 62 }
)

$nl = [Environment]::NewLine
$sb = [System.Text.StringBuilder]::new(5200000)
[void]$sb.Append('#include "lvgl.h"' + $nl)
[void]$sb.Append('#ifndef LV_ATTRIBUTE_MEM_ALIGN' + $nl + '#define LV_ATTRIBUTE_MEM_ALIGN' + $nl + '#endif' + $nl)
[void]$sb.Append('#ifndef LV_ATTRIBUTE_LARGE_CONST' + $nl + '#define LV_ATTRIBUTE_LARGE_CONST' + $nl + '#endif' + $nl + $nl)
[void]$sb.Append('#if LV_COLOR_DEPTH != 16 || LV_COLOR_16_SWAP != 0' + $nl)
[void]$sb.Append('#error "Heavy Rain home expects RGB565 with LV_COLOR_16_SWAP=0"' + $nl + '#endif' + $nl + $nl)

function Convert-BitmapPixels([System.Drawing.Bitmap]$Bitmap, [bool]$WithAlpha) {
    $rect = [System.Drawing.Rectangle]::new(0, 0, $Bitmap.Width, $Bitmap.Height)
    $bits = $Bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $raw = [byte[]]::new($bits.Stride * $Bitmap.Height)
        [System.Runtime.InteropServices.Marshal]::Copy($bits.Scan0, $raw, 0, $raw.Length)
        $bytesPerPixel = if ($WithAlpha) { 3 } else { 2 }
        $out = [byte[]]::new($Bitmap.Width * $Bitmap.Height * $bytesPerPixel)
        $dst = 0
        for ($y = 0; $y -lt $Bitmap.Height; $y++) {
            for ($x = 0; $x -lt $Bitmap.Width; $x++) {
                $src = $y * $bits.Stride + $x * 4
                $r = [int]$raw[$src + 2]
                $g = [int]$raw[$src + 1]
                $b = [int]$raw[$src]
                $rgb565 = (($r -band 0xF8) -shl 8) -bor (($g -band 0xFC) -shl 3) -bor ($b -shr 3)
                # LVGL reads true-color alpha pixels as RGB565 little-endian
                # followed by alpha: low byte, high byte, alpha.  Keeping alpha
                # last is essential; putting it first turns the portrait into
                # noisy, partly transparent colour fragments on device.
                if ($WithAlpha) {
                    $out[$dst++] = [byte]($rgb565 -band 255)
                    $out[$dst++] = [byte](($rgb565 -shr 8) -band 255)
                    $out[$dst++] = [byte]$raw[$src + 3]
                }
                else {
                    $out[$dst++] = [byte]($rgb565 -band 255)
                    $out[$dst++] = [byte](($rgb565 -shr 8) -band 255)
                }
            }
        }
        return ,$out
    }
    finally { $Bitmap.UnlockBits($bits) }
}

function Append-ImageDescriptor([System.Text.StringBuilder]$Builder, [string]$Name, [byte[]]$Pixels, [int]$Width, [int]$Height, [bool]$WithAlpha) {
    $type = if ($WithAlpha) { 'LV_IMG_CF_TRUE_COLOR_ALPHA' } else { 'LV_IMG_CF_TRUE_COLOR' }
    [void]$Builder.Append("static const LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST uint8_t ${Name}_map[] = {" + $nl)
    for ($i = 0; $i -lt $Pixels.Length; $i++) {
        [void]$Builder.AppendFormat('0x{0:x2},', [int]$Pixels[$i])
        if (($i + 1) % 24 -eq 0) { [void]$Builder.Append($nl) }
    }
    [void]$Builder.Append($nl + '};' + $nl)
    [void]$Builder.Append("const lv_img_dsc_t ${Name} = {" + $nl)
    [void]$Builder.Append(" .header.cf = ${type}," + $nl)
    [void]$Builder.Append(' .header.always_zero = 0,' + $nl + ' .header.reserved = 0,' + $nl)
    [void]$Builder.Append(" .header.w = ${Width}," + $nl + " .header.h = ${Height}," + $nl)
    [void]$Builder.Append(" .data_size = sizeof(${Name}_map)," + $nl + " .data = ${Name}_map," + $nl + '};' + $nl + $nl)
}

function New-CharacterColorGrade([single]$Opacity = 1.0, [int]$AmbientR = -1, [int]$AmbientG = -1, [int]$AmbientB = -1, [single]$AmbientMix = 0.0) {
    $matrix = [System.Drawing.Imaging.ColorMatrix]::new()
    # Preserve the illustration's own flat colours.  Only a small common cast is
    # added so the portrait belongs to the scene instead of becoming blue-grey.
    $keep = [single]0.92
    $tint = [single]0.08
    $ambient = [single]0
    if ($AmbientR -ge 0 -and $AmbientG -ge 0 -and $AmbientB -ge 0) {
        $ambient = $AmbientMix
        $keep *= (1.0 - $ambient)
        $tint *= (1.0 - $ambient)
    }
    $matrix.Matrix00 = $keep
    $matrix.Matrix11 = $keep
    $matrix.Matrix22 = $keep
    $matrix.Matrix33 = $Opacity
    $matrix.Matrix40 = $tint * (221.0 / 255.0) + $ambient * ($AmbientR / 255.0)
    $matrix.Matrix41 = $tint * (213.0 / 255.0) + $ambient * ($AmbientG / 255.0)
    $matrix.Matrix42 = $tint * (193.0 / 255.0) + $ambient * ($AmbientB / 255.0)
    $attributes = [System.Drawing.Imaging.ImageAttributes]::new()
    $attributes.SetColorMatrix($matrix, [System.Drawing.Imaging.ColorMatrixFlag]::Default, [System.Drawing.Imaging.ColorAdjustType]::Bitmap)
    return $attributes
}

function New-CharacterShadow([single]$Opacity = 0.18) {
    $matrix = [System.Drawing.Imaging.ColorMatrix]::new()
    $matrix.Matrix00 = 0; $matrix.Matrix11 = 0; $matrix.Matrix22 = 0
    $matrix.Matrix33 = $Opacity
    $matrix.Matrix40 = 23.0 / 255.0
    $matrix.Matrix41 = 40.0 / 255.0
    $matrix.Matrix42 = 59.0 / 255.0
    $attributes = [System.Drawing.Imaging.ImageAttributes]::new()
    $attributes.SetColorMatrix($matrix, [System.Drawing.Imaging.ColorMatrixFlag]::Default, [System.Drawing.Imaging.ColorAdjustType]::Bitmap)
    return $attributes
}

function Apply-BottomFeather([System.Drawing.Bitmap]$Bitmap, [int]$FadeHeight = 14) {
    $rect = [System.Drawing.Rectangle]::new(0, 0, $Bitmap.Width, $Bitmap.Height)
    $bits = $Bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadWrite, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $raw = [byte[]]::new($bits.Stride * $Bitmap.Height)
        [System.Runtime.InteropServices.Marshal]::Copy($bits.Scan0, $raw, 0, $raw.Length)
        $startY = [Math]::Max(0, $Bitmap.Height - $FadeHeight)
        for ($y = $startY; $y -lt $Bitmap.Height; $y++) {
            $t = ($y - $startY) / [single][Math]::Max(1, ($Bitmap.Height - 1 - $startY))
            $alphaFactor = 1.0 - ($t * $t * (3.0 - 2.0 * $t))
            for ($x = 0; $x -lt $Bitmap.Width; $x++) {
                $index = $y * $bits.Stride + $x * 4 + 3
                $raw[$index] = [byte][Math]::Round($raw[$index] * $alphaFactor)
            }
        }
        [System.Runtime.InteropServices.Marshal]::Copy($raw, 0, $bits.Scan0, $raw.Length)
    }
    finally { $Bitmap.UnlockBits($bits) }
}

function Get-AlphaBounds([System.Drawing.Image]$Image) {
    $left = $Image.Width; $top = $Image.Height; $right = -1; $bottom = -1
    for ($y = 0; $y -lt $Image.Height; $y++) {
        for ($x = 0; $x -lt $Image.Width; $x++) {
            if ($Image.GetPixel($x, $y).A -gt 12) {
                if ($x -lt $left) { $left = $x }; if ($x -gt $right) { $right = $x }
                if ($y -lt $top) { $top = $y }; if ($y -gt $bottom) { $bottom = $y }
            }
        }
    }
    if ($right -lt $left -or $bottom -lt $top) { throw 'Character image has no visible alpha pixels.' }
    return [System.Drawing.Rectangle]::new($left, $top, $right - $left + 1, $bottom - $top + 1)
}

function Apply-PortraitFeather([System.Drawing.Bitmap]$Bitmap, [int]$RightFade = 30, [int]$BottomFade = 10) {
    $rect = [System.Drawing.Rectangle]::new(0, 0, $Bitmap.Width, $Bitmap.Height)
    $bits = $Bitmap.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadWrite, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    try {
        $raw = [byte[]]::new($bits.Stride * $Bitmap.Height)
        [System.Runtime.InteropServices.Marshal]::Copy($bits.Scan0, $raw, 0, $raw.Length)
        for ($y = 0; $y -lt $Bitmap.Height; $y++) {
            for ($x = 0; $x -lt $Bitmap.Width; $x++) {
                $factor = 1.0
                $rightStart = $Bitmap.Width - $RightFade
                if ($x -ge $rightStart) {
                    $t = ($x - $rightStart) / [single][Math]::Max(1, ($Bitmap.Width - 1 - $rightStart))
                    $factor *= 1.0 - ($t * $t * (3.0 - 2.0 * $t))
                }
                $bottomStart = $Bitmap.Height - $BottomFade
                if ($y -ge $bottomStart) {
                    $t = ($y - $bottomStart) / [single][Math]::Max(1, ($Bitmap.Height - 1 - $bottomStart))
                    $factor *= 1.0 - ($t * $t * (3.0 - 2.0 * $t))
                }
                $index = $y * $bits.Stride + $x * 4 + 3
                $raw[$index] = [byte][Math]::Round($raw[$index] * $factor)
            }
        }
        [System.Runtime.InteropServices.Marshal]::Copy($raw, 0, $bits.Scan0, $raw.Length)
    }
    finally { $Bitmap.UnlockBits($bits) }
}

$characterImageForBounds = [System.Drawing.Image]::FromFile($characterSource)
$characterBounds = Get-AlphaBounds $characterImageForBounds
$characterImageForBounds.Dispose()

foreach ($scene in $scenes) {
    $source = [System.Drawing.Image]::FromFile((Join-Path $assetRoot 'poster-base.png'))
    $background = [System.Drawing.Bitmap]::new(320, 240, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $bgGraphics = [System.Drawing.Graphics]::FromImage($background)
    $bgGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $bgGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
    $bgGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    $targetRatio = 4.0 / 3.0
    $sourceRatio = $source.Width / [double]$source.Height
    if ($sourceRatio -gt $targetRatio) {
        $cropWidth = [int]($source.Height * $targetRatio)
        $sourceRect = [System.Drawing.Rectangle]::new([int](($source.Width - $cropWidth) / 2), 0, $cropWidth, $source.Height)
    }
    else {
        $cropHeight = [int]($source.Width / $targetRatio)
        $sourceRect = [System.Drawing.Rectangle]::new(0, [int](($source.Height - $cropHeight) / 2), $source.Width, $cropHeight)
    }
    $bgGraphics.DrawImage($source, [System.Drawing.Rectangle]::new(0, 0, 320, 240), $sourceRect, [System.Drawing.GraphicsUnit]::Pixel)
    $sceneWash = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb($scene.BgAlpha, $scene.BgR, $scene.BgG, $scene.BgB))
    $bgGraphics.FillRectangle($sceneWash, 0, 0, 320, 240)
    $sceneWash.Dispose()
    $source.Dispose()
    $background.Save((Join-Path $assetRoot $scene.File), [System.Drawing.Imaging.ImageFormat]::Png)
    $backgroundPixels = Convert-BitmapPixels $background $false
    Append-ImageDescriptor $sb "heavy_rain_$($scene.Name)_bg" $backgroundPixels 320 240 $false

    $preview = [System.Drawing.Bitmap]::new($background)
    $g = [System.Drawing.Graphics]::FromImage($preview)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $portrait = [System.Drawing.Image]::FromFile($characterSource)
    $portraitBitmapPreview = [System.Drawing.Bitmap]::new(174, 240, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $portraitGradeGraphics = [System.Drawing.Graphics]::FromImage($portraitBitmapPreview)
    $portraitGradeGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $portraitAttributes = New-CharacterColorGrade 1.0 $scene.TintR $scene.TintG $scene.TintB $scene.TintMix
    $portraitScale = [Math]::Min(174.0 / $characterBounds.Width, 238.0 / $characterBounds.Height)
    $portraitWidth = [int][Math]::Round($characterBounds.Width * $portraitScale)
    $portraitHeight = [int][Math]::Round($characterBounds.Height * $portraitScale)
    $portraitX = [int][Math]::Floor((174 - $portraitWidth) / 2.0)
    $portraitY = 240 - $portraitHeight
    $portraitGradeGraphics.DrawImage($portrait, [System.Drawing.Rectangle]::new($portraitX, $portraitY, $portraitWidth, $portraitHeight), $characterBounds.X, $characterBounds.Y, $characterBounds.Width, $characterBounds.Height, [System.Drawing.GraphicsUnit]::Pixel, $portraitAttributes)
    $portraitAttributes.Dispose(); $portraitGradeGraphics.Dispose()
    # Keep the complete, clean alpha silhouette. Feathering had erased the
    # outer strands and shoulder edge, making the bust look incomplete.
    $g.DrawImage($portraitBitmapPreview, [System.Drawing.Rectangle]::new(-8, 0, 174, 240))
    $portraitBitmapPreview.Dispose(); $portrait.Dispose()
    $previewHour = [int]$scene.Time.Substring(0, 2)
    if ((($previewHour -ge 17) -and ($previewHour -lt 20)) -or (($previewHour -ge 5) -and ($previewHour -lt 6))) {
        $timeBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(77, 167, 122, 120))
        $g.FillRectangle($timeBrush, 0, 0, 320, 240)
        $timeBrush.Dispose()
    }
    elseif (($previewHour -ge 20) -or ($previewHour -lt 5)) {
        $timeBrush = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(127, 16, 24, 49))
        $g.FillRectangle($timeBrush, 0, 0, 320, 240)
        $timeBrush.Dispose()
    }
    $font8 = [System.Drawing.Font]::new('Arial', 8, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $font10 = [System.Drawing.Font]::new('Arial', 10, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $font12 = [System.Drawing.Font]::new('Bahnschrift SemiBold', 12, [System.Drawing.FontStyle]::Italic, [System.Drawing.GraphicsUnit]::Pixel)
    $font16 = [System.Drawing.Font]::new('Microsoft YaHei UI', 16, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $timeFont = [System.Drawing.Font]::new('Arial', 40, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $tempFont = [System.Drawing.Font]::new('Bahnschrift SemiBold', 30, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $small = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 184, 196, 204))
    $white = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 239, 231, 216))
    $gold = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 230, 183, 93))
    $cyan = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 66, 189, 232))
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $g.DrawLine([System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(100, 230, 183, 93), 1), 166, 14, 166, 228)
    $g.FillRectangle($gold, 12, 15, 18, 2)
    $g.DrawString('HEAVY RAIN', $font12, $white, 12, 22)
    $g.DrawString('RHODES ISLAND', $font12, $small, 12, 38)
    $hourText = $scene.Time.Substring(0, 2); $minuteText = $scene.Time.Substring(3, 2)
    $hourWidth = $g.MeasureString($hourText, $timeFont).Width
    $colonWidth = $g.MeasureString(':', $timeFont).Width
    $minuteWidth = $g.MeasureString($minuteText, $timeFont).Width
    $clockX = 168
    $g.DrawString($hourText, $timeFont, $white, $clockX, 0)
    $g.DrawString(':', $timeFont, $cyan, $clockX + $hourWidth, 0)
    $g.DrawString($minuteText, $timeFont, $white, $clockX + $hourWidth + $colonWidth, 0)
    $dateText = '11-16 MON'
    $g.DrawString($dateText, $font12, $small, 172, 44)
    if ($scene.Name -eq 'sunny') {
        $sun = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 230, 183, 93))
        $sunPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 230, 183, 93), 2)
        $g.FillEllipse($sun, 183, 88, 14, 14)
        $g.DrawLine($sunPen, 190, 74, 190, 81); $g.DrawLine($sunPen, 190, 109, 190, 116)
        $g.DrawLine($sunPen, 169, 95, 176, 95); $g.DrawLine($sunPen, 204, 95, 211, 95)
        $g.DrawLine($sunPen, 175, 80, 180, 85); $g.DrawLine($sunPen, 200, 105, 205, 110)
        $g.DrawLine($sunPen, 175, 110, 180, 105); $g.DrawLine($sunPen, 200, 85, 205, 80)
        $sun.Dispose(); $sunPen.Dispose()
    } else {
        $cloud = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 204, 215, 220))
        $g.FillEllipse($cloud, 174, 89, 17, 16); $g.FillEllipse($cloud, 182, 80, 18, 20); $g.FillEllipse($cloud, 193, 89, 16, 15); $g.FillRectangle($cloud, 179, 93, 25, 12)
        $cloud.Dispose()
        if ($scene.Name -eq 'rain') {
            $rainPen = [System.Drawing.Pen]::new([System.Drawing.Color]::FromArgb(255, 126, 190, 205), 1)
            $g.DrawLine($rainPen, 181, 106, 178, 113); $g.DrawLine($rainPen, 190, 106, 187, 113); $g.DrawLine($rainPen, 200, 106, 197, 113)
            $rainPen.Dispose()
        } elseif ($scene.Name -eq 'snow') {
            $snow = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 223, 232, 237))
            $g.FillEllipse($snow, 179, 109, 4, 4); $g.FillEllipse($snow, 190, 111, 4, 4); $g.FillEllipse($snow, 201, 109, 4, 4)
            $snow.Dispose()
        }
    }
    $g.DrawString('0°', $tempFont, $gold, 216, 77)
    $g.DrawString($scene.Caption, $font16, $white, 172, 122)
    $g.DrawString('北京', $font16, $small, 172, 144)
    $g.DrawString('BATTERY', $font12, $small, 172, 169)
    $g.DrawString('82%', $font16, $white, 270, 165)
    $track = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 79, 94, 105))
    $fill = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 230, 183, 93))
    $g.FillRectangle($track, 172, 188, 132, 3); $g.FillRectangle($fill, 172, 188, 108, 3)
    $g.DrawString('2.4G', $font12, $gold, 172, 207)
    $g.DrawString('MAC', $font12, $white, 220, 207)
    $dot = [System.Drawing.SolidBrush]::new([System.Drawing.Color]::FromArgb(255, 230, 183, 93))
    $g.FillEllipse($dot, 261, 214, 4, 4); $dot.Dispose()
    $g.DrawString('WAIT', $font12, $white, 270, 207)
    $previewPath = Join-Path $PSScriptRoot ("heavy-rain-weather-{0}-preview.png" -f $scene.Name)
    $preview.Save($previewPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $font8.Dispose(); $font10.Dispose(); $font12.Dispose(); $font16.Dispose(); $timeFont.Dispose(); $tempFont.Dispose()
    $small.Dispose(); $white.Dispose(); $gold.Dispose(); $cyan.Dispose(); $track.Dispose(); $fill.Dispose()
    $g.Dispose(); $preview.Dispose()

    $background.Dispose(); $bgGraphics.Dispose()
    Write-Output "Preview: $previewPath"
}

$portraitSource = [System.Drawing.Image]::FromFile($characterSource)
    $portraitBitmap = [System.Drawing.Bitmap]::new(174, 240, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$portraitGraphics = [System.Drawing.Graphics]::FromImage($portraitBitmap)
$portraitGraphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$portraitGraphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
$portraitGraphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
$portraitAttributes = New-CharacterColorGrade 1.0
$portraitScale = [Math]::Min(174.0 / $characterBounds.Width, 238.0 / $characterBounds.Height)
$portraitWidth = [int][Math]::Round($characterBounds.Width * $portraitScale)
$portraitHeight = [int][Math]::Round($characterBounds.Height * $portraitScale)
$portraitX = [int][Math]::Floor((174 - $portraitWidth) / 2.0)
$portraitY = 240 - $portraitHeight
$portraitGraphics.DrawImage($portraitSource, [System.Drawing.Rectangle]::new($portraitX, $portraitY, $portraitWidth, $portraitHeight), $characterBounds.X, $characterBounds.Y, $characterBounds.Width, $characterBounds.Height, [System.Drawing.GraphicsUnit]::Pixel, $portraitAttributes)
$portraitAttributes.Dispose()
$portraitPixels = Convert-BitmapPixels $portraitBitmap $true
Append-ImageDescriptor $sb 'heavy_rain_character' $portraitPixels 174 240 $true
$portraitGraphics.Dispose(); $portraitBitmap.Dispose(); $portraitSource.Dispose()

$outputPath = Join-Path $root 'main\heavy_rain_home_bg.c'
[System.IO.File]::WriteAllText($outputPath, $sb.ToString(), [System.Text.Encoding]::ASCII)
Write-Output "LVGL image assets: $outputPath"
