[CmdletBinding()]
param(
    [string]$BuildDir = (Join-Path $PSScriptRoot '../build'),
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [Parameter(Mandatory = $true)][string]$QtSourceRoot
)

$ErrorActionPreference = 'Stop'
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildPath = (Resolve-Path $BuildDir).Path
$qtPath = (Resolve-Path $QtRoot).Path
$qtSourcePath = (Resolve-Path $QtSourceRoot).Path
$cache = Join-Path $buildPath 'CMakeCache.txt'
$gallery = Join-Path $buildPath 'demo/Release/winui3style_gallery.exe'
$runtime = Join-Path $buildPath 'src/Release/winui3style.dll'
$plugin = Join-Path $buildPath 'plugins/styles/qwinui3style.dll'
$deployTool = Join-Path $qtPath 'bin/windeployqt.exe'
foreach ($required in @($cache, $gallery, $runtime, $plugin, $deployTool,
                       (Join-Path $qtSourcePath 'qtbase/LICENSES/LGPL-3.0-only.txt'))) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing prerequisite: $required"
    }
}
$dirty = @(git -C $repoRoot diff HEAD --numstat)
if ($LASTEXITCODE -ne 0 -or $dirty.Count -ne 0) {
    throw 'Commit tracked source/document changes before packaging (no revision mismatch).'
}
$revision = (git -C $repoRoot rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify source revision.' }
$versionMatch = Select-String -LiteralPath (Join-Path $repoRoot 'CMakeLists.txt') `
    -Pattern 'project\(WinUI3Style VERSION ([0-9.]+)' | Select-Object -First 1
if (-not $versionMatch) { throw 'Cannot identify project version.' }
$version = $versionMatch.Matches[0].Groups[1].Value
$qtVersion = (& (Join-Path $qtPath 'bin/qtpaths.exe') --qt-version).Trim()
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify Qt version.' }
$cmakeLine = Select-String -LiteralPath $cache -Pattern '^CMAKE_COMMAND:INTERNAL=(.+)$'
$cmake = $cmakeLine.Matches[0].Groups[1].Value
$configuredQt = Select-String -LiteralPath $cache -Pattern '^Qt6_DIR:PATH=(.+)$'
if (-not $configuredQt -or
    (Resolve-Path $configuredQt.Matches[0].Groups[1].Value).Path -ne
    (Resolve-Path (Join-Path $qtPath 'lib/cmake/Qt6')).Path) {
    throw 'QtRoot must match the Qt 6 installation used by the configured build.'
}
if (-not (Select-String -LiteralPath $cache -Pattern '^CMAKE_GENERATOR_PLATFORM:INTERNAL=x64$')) {
    throw 'This package requires a configured Visual Studio x64 build.'
}
# Rebuild the committed revision; never bundle binaries from an earlier checkout.
& $cmake -S $repoRoot -B $buildPath
if ($LASTEXITCODE -ne 0) { throw 'Release configuration failed.' }
& $cmake --build $buildPath --config Release
if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }

$name = "WinUI3Style-$version-win-x64-msvc-qt$qtVersion-experimental"
$output = Join-Path $buildPath ('poc/' + [guid]::NewGuid().ToString())
$stage = Join-Path $output $name
New-Item -ItemType Directory -Path (Join-Path $stage 'styles') -Force | Out-Null
Copy-Item -LiteralPath $gallery, $runtime -Destination $stage
Copy-Item -LiteralPath $plugin -Destination (Join-Path $stage 'styles')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'poc/qt.conf') -Destination $stage
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'poc/README.md') -Destination $stage

$savedPath = $env:Path
try {
    $env:Path = "$(Join-Path $qtPath 'bin');$savedPath"
    & $deployTool --release --no-translations --no-compiler-runtime `
        --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw `
        --skip-plugin-types networkinformation,tls `
        --qtpaths (Join-Path $qtPath 'bin/qtpaths.exe') `
        (Join-Path $stage 'winui3style_gallery.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Qt runtime deployment failed.' }
} finally {
    $env:Path = $savedPath
}
# Include the small offscreen platform for reproducible packaged-capture checks.
Copy-Item -LiteralPath (Join-Path $qtPath 'plugins/platforms/qoffscreen.dll') `
    -Destination (Join-Path $stage 'platforms')
$licenses = Join-Path $stage 'licenses'
New-Item -ItemType Directory -Path $licenses -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'LICENSE') `
    -Destination (Join-Path $licenses 'WinUI3Style-LGPL-2.1.txt')
Copy-Item -LiteralPath (Join-Path $repoRoot 'LICENSES/README.md') `
    -Destination (Join-Path $licenses 'WinUI3Style-NOTICE.md')
foreach ($module in @('qtbase', 'qtsvg', 'qtimageformats')) {
    $modulePath = Join-Path $qtSourcePath $module
    $moduleNotices = Join-Path $licenses $module
    New-Item -ItemType Directory -Path $moduleNotices -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $modulePath 'LICENSES') -Destination $moduleNotices -Recurse
    $attributions = Get-ChildItem -LiteralPath $modulePath -Recurse -File -Filter qt_attribution.json
    foreach ($attribution in $attributions) {
        $relative = $attribution.FullName.Substring($modulePath.Length).TrimStart('\', '/')
        $target = Join-Path $moduleNotices $relative
        New-Item -ItemType Directory -Path (Split-Path $target -Parent) -Force | Out-Null
        Copy-Item -LiteralPath $attribution.FullName -Destination $target
        # Attribution-specific license files contain third-party copyright notices.
        $entries = @(Get-Content -LiteralPath $attribution.FullName -Raw | ConvertFrom-Json)
        foreach ($entry in $entries) {
            foreach ($licenseFile in @($entry.LicenseFile)) {
                if (-not $licenseFile) { continue }
                $licenseSource = Join-Path $attribution.DirectoryName $licenseFile
                if (-not (Test-Path -LiteralPath $licenseSource -PathType Leaf)) {
                    throw "Missing third-party notice: $licenseSource"
                }
                $noticeTarget = Join-Path (Split-Path $target -Parent) $licenseFile
                $noticeTarget = [System.IO.Path]::GetFullPath($noticeTarget)
                if (-not $noticeTarget.StartsWith($licenses + [System.IO.Path]::DirectorySeparatorChar,
                                                 [System.StringComparison]::OrdinalIgnoreCase)) {
                    throw "Third-party notice escapes the package: $licenseFile"
                }
                New-Item -ItemType Directory -Path (Split-Path $noticeTarget -Parent) -Force | Out-Null
                Copy-Item -LiteralPath $licenseSource -Destination $noticeTarget
            }
        }
    }
    $sbom = Join-Path $qtPath "sbom/$module-$qtVersion.spdx"
    if (Test-Path -LiteralPath $sbom) { Copy-Item -LiteralPath $sbom -Destination $licenses }
}
& git -C $repoRoot archive --format=zip "--output=$(Join-Path $stage 'WinUI3Style-source.zip')" HEAD
if ($LASTEXITCODE -ne 0) { throw 'Source archive failed.' }
$savedPluginPath = $env:QT_PLUGIN_PATH
$savedQpa = $env:QT_QPA_PLATFORM
try {
    $env:Path = "$env:SystemRoot/System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = $null
    $env:QT_QPA_PLATFORM = 'windows'
    # Qt displays --help in a modal Windows message box without console handles.
    # Redirecting both streams sets STARTF_USESTDHANDLES and makes this unattended.
    $smoke = Start-Process -FilePath (Join-Path $stage 'winui3style_gallery.exe') `
        -ArgumentList '--help' -WorkingDirectory $stage -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $output 'startup.stdout.txt') `
        -RedirectStandardError (Join-Path $output 'startup.stderr.txt')
    if (-not $smoke.WaitForExit(30000)) {
        $smoke.Kill()
        throw 'Packaged startup timed out (possible missing DLL/platform plugin).'
    }
    $smoke.Refresh()
    if ($smoke.ExitCode -ne 0) { throw "Packaged startup failed: $($smoke.ExitCode)" }
    if (-not (Select-String -LiteralPath (Join-Path $output 'startup.stdout.txt') `
                           -SimpleMatch '--capture-dir')) {
        throw 'Packaged executable did not produce the expected gallery help.'
    }
} finally {
    $env:Path = $savedPath
    $env:QT_PLUGIN_PATH = $savedPluginPath
    $env:QT_QPA_PLATFORM = $savedQpa
}
$files = @(Get-ChildItem -LiteralPath $stage -Recurse -File | Sort-Object FullName | ForEach-Object {
    [ordered]@{
        path = $_.FullName.Substring($stage.Length + 1).Replace('\', '/')
        sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash
    }
})
[ordered]@{
    name = $name
    revision = $revision
    qtVersion = $qtVersion
    status = 'local-candidate-not-publication-verified'
    compilerRuntime = 'Install current Microsoft Visual C++ v14 x64 Redistributable if absent'
    localStartup = 'Windows platform, Windows-only PATH, QT_PLUGIN_PATH cleared: passed'
    qtCorrespondingSource = 'Exact compiled Qt source/modifications still require release verification'
    files = $files
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stage 'manifest.json') -Encoding utf8
$zip = Join-Path $output ($name + '.zip')
Compress-Archive -LiteralPath $stage -DestinationPath $zip -CompressionLevel Optimal
$checksum = Get-FileHash -LiteralPath $zip -Algorithm SHA256
"$($checksum.Hash)  $([System.IO.Path]::GetFileName($zip))" | Set-Content -LiteralPath ($zip + '.sha256')
Write-Host "Local PoC candidate: $zip"
Write-Host 'Before publication: clean-machine test, exact-revision hosted CI, matching Qt corresponding sources.'
