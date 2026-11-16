[CmdletBinding()]
param(
    [string]$BuildDir = (Join-Path $PSScriptRoot '../build'),
    [Parameter(Mandatory = $true)][string]$QtRoot,
    [switch]$ExcludeNativeTests,
    [switch]$SkipTests,
    [ValidatePattern('^([A-Za-z0-9][A-Za-z0-9.-]*)?$')][string]$ReleaseSuffix = ''
)

$ErrorActionPreference = 'Stop'
$policyHelper = Join-Path $PSScriptRoot 'package_ctest_policy.ps1'
. $policyHelper
$repoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$buildPath = (Resolve-Path $BuildDir).Path
$qtPath = (Resolve-Path $QtRoot).Path
$cache = Join-Path $buildPath 'CMakeCache.txt'
$gallery = Join-Path $buildPath 'src/gallery/Release/winui3style_gallery.exe'
# The style implementation is embedded in qwinui3style.dll (issue #70), so the
# package ships one project DLL under styles/ and no winui3style.dll companion.
$plugin = Join-Path $buildPath 'plugins/styles/qwinui3style.dll'
$modernWindowsStylePlugin = Join-Path $qtPath 'plugins/styles/qmodernwindowsstyle.dll'
$deployTool = Join-Path $qtPath 'bin/windeployqt.exe'
foreach ($required in @($cache, $gallery, $plugin, $modernWindowsStylePlugin, $deployTool,
                       (Join-Path $qtPath 'plugins/styles/qmodernwindowsstyle.dll'))) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Missing prerequisite: $required"
    }
}
$dirty = @(git -C $repoRoot status --porcelain --untracked-files=all)
if ($LASTEXITCODE -ne 0 -or $dirty.Count -ne 0) {
    throw 'Commit all tracked and untracked source/document changes before packaging (no revision mismatch).'
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
if (-not $SkipTests) {
    $ctest = Join-Path (Split-Path $cmake -Parent) 'ctest.exe'
    if (-not (Test-Path -LiteralPath $ctest -PathType Leaf)) { throw 'CTest is required for packaging.' }
    $ctestArgs = Get-PackageCTestArguments -BuildPath $buildPath `
        -ExcludeNativeTests:$ExcludeNativeTests
    & $ctest @ctestArgs
    if ($LASTEXITCODE -ne 0) { throw 'Release tests failed; no distribution archive created.' }
}

$name = "WinUI3Style-$version-win-x64-msvc-qt$qtVersion"
if (-not [string]::IsNullOrEmpty($ReleaseSuffix)) {
    $name = "$name-$ReleaseSuffix"
}
$output = Join-Path $buildPath ('poc/' + [guid]::NewGuid().ToString())
$stage = Join-Path $output $name
New-Item -ItemType Directory -Path (Join-Path $stage 'styles') -Force | Out-Null
Copy-Item -LiteralPath $gallery -Destination $stage
Copy-Item -LiteralPath $plugin -Destination (Join-Path $stage 'styles')
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'poc/README.md') -Destination $stage

$savedPath = $env:Path
try {
    $env:Path = "$(Join-Path $qtPath 'bin');$savedPath"
    & $deployTool --release --no-translations --compiler-runtime `
        --include-plugins qmodernwindowsstyle `
        --no-system-d3d-compiler --no-system-dxc-compiler --no-opengl-sw `
        --skip-plugin-types networkinformation,tls `
        --qtpaths (Join-Path $qtPath 'bin/qtpaths.exe') `
        (Join-Path $stage 'winui3style_gallery.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Qt runtime deployment failed.' }
} finally {
    $env:Path = $savedPath
}
foreach ($deployedPrerequisite in @(
    (Join-Path $stage 'styles/qmodernwindowsstyle.dll'),
    (Join-Path $stage 'vc_redist.x64.exe')
)) {
    if (-not (Test-Path -LiteralPath $deployedPrerequisite -PathType Leaf)) {
        throw "Qt deployment omitted required package file: $deployedPrerequisite"
    }
}
# Include the small offscreen platform for reproducible packaged-capture checks.
Copy-Item -LiteralPath (Join-Path $qtPath 'plugins/platforms/qoffscreen.dll') `
    -Destination (Join-Path $stage 'platforms')
$licenses = Join-Path $stage 'licenses'
New-Item -ItemType Directory -Path $licenses -Force | Out-Null
Copy-Item -LiteralPath (Join-Path $repoRoot 'LICENSE') `
    -Destination (Join-Path $licenses 'WinUI3Style-LGPL-2.1.txt')
# Qt can show GUI-app help in a modal dialog unless all standard handles are
# redirected. Use Process directly so the real exit code remains available.
$smokeStartInfo = [System.Diagnostics.ProcessStartInfo]::new()
$smokeStartInfo.FileName = Join-Path $stage 'winui3style_gallery.exe'
$smokeStartInfo.Arguments = '--help'
$smokeStartInfo.WorkingDirectory = $stage
$smokeStartInfo.UseShellExecute = $false
$smokeStartInfo.RedirectStandardInput = $true
$smokeStartInfo.RedirectStandardOutput = $true
$smokeStartInfo.RedirectStandardError = $true
$smokeStartInfo.EnvironmentVariables['PATH'] = "$env:SystemRoot/System32;$env:SystemRoot"
$smokeStartInfo.EnvironmentVariables['QT_PLUGIN_PATH'] = ''
$smokeStartInfo.EnvironmentVariables['QT_QPA_PLATFORM'] = 'windows'
$smoke = [System.Diagnostics.Process]::new()
$smoke.StartInfo = $smokeStartInfo
if (-not $SkipTests) {
    try {
        if (-not $smoke.Start()) { throw 'Could not start packaged Gallery executable.' }
        $smokeStdout = $smoke.StandardOutput.ReadToEndAsync()
        $smokeStderr = $smoke.StandardError.ReadToEndAsync()
        $smoke.StandardInput.Close()
        if (-not $smoke.WaitForExit(30000)) {
            $smoke.Kill()
            $smoke.WaitForExit()
            throw 'Packaged startup timed out (possible missing DLL/platform plugin).'
        }
        $smoke.WaitForExit()
        $smokeOutput = $smokeStdout.GetAwaiter().GetResult()
        $smokeError = $smokeStderr.GetAwaiter().GetResult()
        [System.IO.File]::WriteAllText((Join-Path $output 'startup.stdout.txt'), $smokeOutput,
                                       [System.Text.UTF8Encoding]::new($false))
        [System.IO.File]::WriteAllText((Join-Path $output 'startup.stderr.txt'), $smokeError,
                                       [System.Text.UTF8Encoding]::new($false))
        if ($smoke.ExitCode -ne 0) { throw "Packaged startup failed: $($smoke.ExitCode)" }
        if (-not $smokeOutput.Contains('--capture-dir')) {
            throw 'Packaged executable did not produce the expected gallery help.'
        }
    } finally {
        $smoke.Dispose()
    }
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
    releaseSuffix = $ReleaseSuffix
    compilerRuntime = [ordered]@{
        strategy = 'Bundled Microsoft Visual C++ v14 x64 redistributable installer'
        installer = 'vc_redist.x64.exe'
        autoInstalled = $false
        guidance = 'Run the installer manually if the target system lacks a compatible x64 v14 runtime'
    }
    stylePlugins = @('styles/qwinui3style.dll', 'styles/qmodernwindowsstyle.dll')
    files = $files
} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $stage 'manifest.json') -Encoding utf8
Write-Output $stage
