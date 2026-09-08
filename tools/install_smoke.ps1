[CmdletBinding()]
param(
    [string]$BuildDir = (Join-Path $PSScriptRoot '..\build'),
    [string]$Configuration = 'Release',
    # Qt's lib/cmake/Qt5 or lib/cmake/Qt6 directory; defaults to the build cache.
    [string]$QtPrefix
)

$ErrorActionPreference = 'Stop'
$buildPath = (Resolve-Path $BuildDir).Path
$cachePath = Join-Path $buildPath 'CMakeCache.txt'
if (-not (Test-Path $cachePath)) { throw "Build directory is not configured: $buildPath" }
$cmakeCommand = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $cmakeCommand) {
    $cmakeLine = Select-String -Path $cachePath -Pattern '^CMAKE_COMMAND:INTERNAL=(.+)$' | Select-Object -First 1
    if ($cmakeLine) { $cmakeCommand = $cmakeLine.Matches[0].Groups[1].Value }
}
if (-not $cmakeCommand -or -not (Test-Path $cmakeCommand)) { throw 'CMake was not found; add it to PATH or configure the build with CMake.' }

if (-not $QtPrefix) {
    $qtDirLine = Select-String -Path $cachePath -Pattern '^Qt[56]_DIR:PATH=(.+)$' | Select-Object -First 1
    if ($qtDirLine) { $QtPrefix = $qtDirLine.Matches[0].Groups[1].Value }
}
if (-not $QtPrefix) { throw 'Pass -QtPrefix pointing at the Qt CMake package directory.' }
$QtPrefix = (Resolve-Path $QtPrefix).Path
$qtRoot = (Resolve-Path (Join-Path $QtPrefix '..\..\..')).Path
$qtPackage = Split-Path $QtPrefix -Leaf
if ($qtPackage -notin @('Qt5', 'Qt6')) { throw "QtPrefix must name a Qt5 or Qt6 CMake package directory: $QtPrefix" }

$tempRoot = Join-Path ([System.IO.Path]::GetTempPath()) ('winui3style-install-smoke-' + [guid]::NewGuid())
$stage = Join-Path $tempRoot 'stage'
$consumer = Join-Path $tempRoot 'consumer'
$consumerBuild = Join-Path $tempRoot 'consumer-build'
New-Item -ItemType Directory -Force $consumer | Out-Null
$savedQtQpa = $env:QT_QPA_PLATFORM
$savedQtPluginPath = $env:QT_PLUGIN_PATH
$savedPath = $env:Path
try {
    & $cmakeCommand --install $buildPath --config $Configuration --prefix $stage
    if ($LASTEXITCODE -ne 0) { throw "cmake --install failed with exit code $LASTEXITCODE" }
    $plugin = Get-ChildItem (Join-Path $stage 'plugins\styles') -Filter 'qwinui3style*.dll' | Select-Object -First 1
    $styleDll = Get-ChildItem (Join-Path $stage 'bin') -Filter 'winui3style*.dll' | Select-Object -First 1
    if (-not $plugin) { throw "staged style plugin is missing under $stage\plugins\styles" }
    if (-not $styleDll) { throw "staged style runtime is missing under $stage\bin" }

    $source = @'
#include <QApplication>
#include <QComboBox>
#include <QPushButton>
#include <QStyle>
#include <QStyleFactory>

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    if (argc != 2)
        return 5;
    // QPA is already loaded. Resolve style factories only from this installation.
    QCoreApplication::setLibraryPaths({QString::fromLocal8Bit(argv[1])});
    QStyle *style = QStyleFactory::create(QStringLiteral("winui3"));
    if (!style || style->objectName().compare(QStringLiteral("winui3"), Qt::CaseInsensitive) != 0)
        return 2;
    QStyle *compact = QStyleFactory::create(QStringLiteral("winui3compact"));
    if (!compact || compact->objectName().compare(QStringLiteral("winui3compact"), Qt::CaseInsensitive) != 0)
        return 4;
    delete compact;
    app.setStyle(style);
    QPushButton button(QStringLiteral("Install smoke"));
    QComboBox combo;
    combo.addItems({QStringLiteral("one"), QStringLiteral("two")});
    button.ensurePolished();
    combo.ensurePolished();
    return (button.sizeHint().isValid() && combo.sizeHint().isValid()) ? 0 : 3;
}
'@
    Set-Content -LiteralPath (Join-Path $consumer 'main.cpp') -Value $source -Encoding utf8
    $project = @"
cmake_minimum_required(VERSION 3.21)
project(winui3style_install_smoke LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
find_package($qtPackage REQUIRED COMPONENTS Core Gui Widgets)
set(QT_MAJOR $($qtPackage.Substring(2)))
add_executable(winui3style_install_smoke main.cpp)
target_link_libraries(winui3style_install_smoke PRIVATE Qt$($qtPackage.Substring(2))::Widgets)
"@
    Set-Content -LiteralPath (Join-Path $consumer 'CMakeLists.txt') -Value $project -Encoding utf8

    & $cmakeCommand -S $consumer -B $consumerBuild "-DCMAKE_PREFIX_PATH=$qtRoot" "-D${qtPackage}_DIR=$QtPrefix"
    if ($LASTEXITCODE -ne 0) { throw "consumer configure failed with exit code $LASTEXITCODE" }
    & $cmakeCommand --build $consumerBuild --config $Configuration
    if ($LASTEXITCODE -ne 0) { throw "consumer build failed with exit code $LASTEXITCODE" }

    $exe = Join-Path $consumerBuild "$Configuration\winui3style_install_smoke.exe"
    if (-not (Test-Path $exe)) { throw "consumer executable was not produced: $exe" }
    $qtBin = Join-Path $qtRoot 'bin'
    $env:QT_QPA_PLATFORM = 'offscreen'
    $env:QT_PLUGIN_PATH = Join-Path $stage 'plugins'
    $env:Path = "$(Join-Path $stage 'bin');$qtBin;$env:Path"
    & $exe (Join-Path $stage 'plugins')
    if ($LASTEXITCODE -ne 0) { throw "consumer runtime smoke failed with exit code $LASTEXITCODE" }
    Write-Host "Install smoke passed: $stage"
}
finally {
    $env:QT_QPA_PLATFORM = $savedQtQpa
    $env:QT_PLUGIN_PATH = $savedQtPluginPath
    $env:Path = $savedPath
    if (Test-Path $tempRoot) {
        $resolvedTemp = (Resolve-Path $tempRoot).Path
        $leaf = Split-Path $resolvedTemp -Leaf
        $expectedParent = (Resolve-Path ([System.IO.Path]::GetTempPath())).Path.TrimEnd('\')
        if ((Split-Path $resolvedTemp -Parent).TrimEnd('\') -eq $expectedParent -and
            $resolvedTemp -eq [System.IO.Path]::GetFullPath($tempRoot) -and
            $leaf -match '^winui3style-install-smoke-[0-9a-f-]{36}$') {
            Remove-Item -LiteralPath $resolvedTemp -Recurse -Force
        } else {
            throw "Refusing to remove unexpected smoke directory: $resolvedTemp"
        }
    }
}
