param(
    [ValidateSet('Light', 'Dark')][string]$Theme = 'Light',
    [string]$Raw = "$env:TEMP\winui3style-demo-$Theme-raw.mp4",
    [string]$QtRoot = 'C:\Qt\6.12.0\msvc2022_64',
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\..\build'),
    [string]$Exe = "$env:TEMP\winui3showcase-build\Release\winui3showcase.exe",
    [string]$Ffmpeg = '',
    [double]$Scale = 2.109375,
    [int]$MonitorX = 0,
    [int]$MonitorY = 0,
    [int]$DurationSeconds = 30,
    [int]$Framerate = 30
)
$ErrorActionPreference = 'Stop'
Get-Process winui3showcase -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Milliseconds 400

Add-Type @'
using System;
using System.Runtime.InteropServices;
public static class ShowCapture3 {
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
    [DllImport("user32.dll")] public static extern bool ClientToScreen(IntPtr h, ref POINT p);
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr h, IntPtr after, int x, int y, int cx, int cy, uint flags);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("user32.dll", SetLastError=true)] public static extern IntPtr OpenInputDesktop(uint flags, bool inherit, uint access);
    [DllImport("user32.dll", CharSet=CharSet.Unicode, SetLastError=true)] public static extern bool GetUserObjectInformation(IntPtr h, int index, System.Text.StringBuilder info, int length, out int needed);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint processId);
    [DllImport("user32.dll", SetLastError=true)] public static extern bool AttachThreadInput(uint from, uint to, bool attach);
    [DllImport("user32.dll")] public static extern bool BringWindowToTop(IntPtr h);
    [DllImport("user32.dll")] public static extern void keybd_event(byte vk, byte scan, uint flags, UIntPtr extra);
    [DllImport("kernel32.dll")] public static extern uint SetThreadExecutionState(uint state);
    [DllImport("user32.dll", SetLastError=true)] public static extern bool SystemParametersInfoW(uint action, uint param, ref int value, uint flags);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }
    [StructLayout(LayoutKind.Sequential)] public struct POINT { public int X, Y; }
}
'@

$desktopHandle = [ShowCapture3]::OpenInputDesktop(0, $false, 1)
$desktopName = New-Object Text.StringBuilder 128
$desktopNeeded = 0
[ShowCapture3]::GetUserObjectInformation($desktopHandle, 2, $desktopName, 128, [ref]$desktopNeeded) | Out-Null
if ($desktopName.ToString() -ne 'Default') { throw "Windows input desktop is '$($desktopName.ToString())'; unlock Windows before capture." }
# The style honours this accessibility preference; recording with it off
# produces a frozen indeterminate progress bar and no transition midpoints.
[int]$clientAnimation = 0
[ShowCapture3]::SystemParametersInfoW(0x1042, 0, [ref]$clientAnimation, 0) | Out-Null
if ($clientAnimation -eq 0) {
  throw 'Windows client-area animation is off (SPI_GETCLIENTAREAANIMATION). Enable Settings > Accessibility > Visual effects > Animation effects before capture.'
}

$qt = $QtRoot
$plugins = Join-Path $BuildDir 'plugins'
$env:PATH = "$qt\bin;$plugins\styles;$env:PATH"
$env:QT_QPA_PLATFORM_PLUGIN_PATH = "$qt\plugins\platforms"
$env:QT_PLUGIN_PATH = "$plugins;$qt\plugins"
$env:QT_SCALE_FACTOR = [string]$Scale
$env:WINUI_DEMO_THEME = $Theme

$app = Start-Process -FilePath $Exe -PassThru
for ($i = 0; $i -lt 60; $i++) {
    Start-Sleep -Milliseconds 100
    $app.Refresh()
    if ($app.MainWindowHandle -ne 0) { break }
}
$h = $app.MainWindowHandle

# Place the window so its client area starts exactly at the monitor origin.
$winX = $MonitorX
$winY = $MonitorY
[ShowCapture3]::SetWindowPos($h, [IntPtr](-1), $winX, $winY, 1936, 1119, 0x0040) | Out-Null
[uint32]$fgPid = 0
[uint32]$targetPid = 0
$fgThread = [ShowCapture3]::GetWindowThreadProcessId([ShowCapture3]::GetForegroundWindow(), [ref]$fgPid)
$targetThread = [ShowCapture3]::GetWindowThreadProcessId($h, [ref]$targetPid)
$attached = $false
if ($fgThread -ne 0 -and $targetThread -ne 0 -and $fgThread -ne $targetThread) {
    $attached = [ShowCapture3]::AttachThreadInput($fgThread, $targetThread, $true)
}
# Synthetic ALT grants foreground rights when a UWP frame host holds focus (AttachThreadInput fails there).
$acquired = $false
for ($try = 0; $try -lt 6 -and -not $acquired; $try++) {
    [ShowCapture3]::keybd_event(0x12, 0, 0, [UIntPtr]::Zero)
    [ShowCapture3]::keybd_event(0x12, 0, 2, [UIntPtr]::Zero)
    [ShowCapture3]::BringWindowToTop($h) | Out-Null
    [ShowCapture3]::SetForegroundWindow($h) | Out-Null
    [ShowCapture3]::SetForegroundWindow($h) | Out-Null
    Start-Sleep -Milliseconds 400
    $acquired = [ShowCapture3]::GetForegroundWindow() -eq $h
}
if ($attached) { [ShowCapture3]::AttachThreadInput($fgThread, $targetThread, $false) | Out-Null }
if (-not $acquired) { throw 'Showcase window did not take foreground; refusing to record.' }

for ($pass = 0; $pass -lt 3; $pass++) {
    $pt = New-Object ShowCapture3+POINT
    [ShowCapture3]::ClientToScreen($h, [ref]$pt) | Out-Null
    $dx = $MonitorX - $pt.X
    $dy = $MonitorY - $pt.Y
    if ($dx -eq 0 -and $dy -eq 0) { break }
    $wr = New-Object ShowCapture3+RECT
    [ShowCapture3]::GetWindowRect($h, [ref]$wr) | Out-Null
    [ShowCapture3]::SetWindowPos($h, [IntPtr](-1), $wr.L + $dx, $wr.T + $dy, 0, 0, 0x0001 -bor 0x0040) | Out-Null
    Start-Sleep -Milliseconds 200
}

$cr = New-Object ShowCapture3+RECT
[ShowCapture3]::GetClientRect($h, [ref]$cr) | Out-Null
$pt = New-Object ShowCapture3+POINT
[ShowCapture3]::ClientToScreen($h, [ref]$pt) | Out-Null
$w = $cr.R
$hgt = $cr.B
"client: $($pt.X),$($pt.Y) ${w}x${hgt} (pid $($app.Id))"
Add-Type -AssemblyName System.Windows.Forms
$vs = [System.Windows.Forms.SystemInformation]::VirtualScreen
if ($pt.X + $w -gt $vs.Right -or $pt.Y + $hgt -gt $vs.Bottom) { throw "client area does not fit the virtual screen" }

if (-not $Ffmpeg) {
    $Ffmpeg = (Get-Command ffmpeg.exe -ErrorAction SilentlyContinue).Source
}
if (-not $Ffmpeg -or -not (Test-Path $Ffmpeg)) {
    throw 'ffmpeg.exe not found; pass -Ffmpeg <path to ffmpeg.exe>.'
}
$raw = $Raw
$size = "${w}x${hgt}"
$args = @(
    '-y', '-f', 'gdigrab', '-framerate', "$Framerate",
    '-video_size', $size, '-offset_x', "$($pt.X)", '-offset_y', "$($pt.Y)",
    '-draw_mouse', '1', '-i', 'desktop', '-t', "$DurationSeconds",
    '-vf', 'crop=trunc(iw/2)*2:trunc(ih/2)*2',
    '-c:v', 'libx264', '-preset', 'medium', '-crf', '16', '-pix_fmt', 'yuv420p', $raw
)
[ShowCapture3]::SetThreadExecutionState([uint32]2147483651) | Out-Null
$rec = Start-Process -FilePath $Ffmpeg -ArgumentList $args -PassThru -WindowStyle Hidden
$rec.WaitForExit()
[ShowCapture3]::SetThreadExecutionState([uint32]2147483648) | Out-Null

Get-Item $raw | Select-Object FullName, Length, LastWriteTime
Stop-Process -Id $app.Id -Force -ErrorAction SilentlyContinue
