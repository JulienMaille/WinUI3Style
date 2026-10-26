# The Windows Server hosted image lacks the Windows 11 icon font used by the
# approved captures. Microsoft permits this download for development/testing,
# not redistribution: keep it on the runner, outside all release assets.
# https://learn.microsoft.com/windows/apps/design/iconography/segoe-fluent-icons-font
$ErrorActionPreference = 'Stop'
$fixture = Join-Path $env:RUNNER_TEMP 'winui3style-visual-font'
New-Item -ItemType Directory -Path $fixture -Force | Out-Null
$archive = Join-Path $fixture 'SegoeFluentIcons.zip'
Invoke-WebRequest 'https://aka.ms/SegoeFluentIcons' -OutFile $archive
if ((Get-FileHash -LiteralPath $archive).Hash -ne '333EDB37CD496F877D5997183FA9900043D39E6F1097DD010B24CB9CB23EC89F') {
    throw 'The pinned Microsoft Segoe Fluent Icons download changed.'
}
Expand-Archive -LiteralPath $archive -DestinationPath $fixture -Force
$font = Join-Path $fixture 'Segoe Fluent Icons.ttf'
if ((Get-FileHash -LiteralPath $font).Hash -ne '031ADA497DDCC9BF3F2536EFBA2F4BD53F12A0DEAD44460A4A934B06A952DD2B') {
    throw 'The pinned Segoe Fluent Icons font changed.'
}
# The offscreen Qt font database loads files directly from QT_QPA_FONTDIR.
Copy-Item -LiteralPath $font -Destination (Join-Path $env:SystemRoot 'Fonts/WinUI3Style-CI-SegoeFluentIcons.ttf') -Force
