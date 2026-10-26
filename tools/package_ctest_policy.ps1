function Get-PackageCTestArguments {
    [CmdletBinding()]
    param(
        [Parameter(Mandatory = $true)][string]$BuildPath,
        [switch]$ExcludeNativeTests
    )

    $arguments = @('--test-dir', $BuildPath, '-C', 'Release', '--output-on-failure', '--no-tests=error')
    if ($ExcludeNativeTests) {
        # Hosted Windows runners have no interactive desktop; follow the same
        # native-test exclusion used by hosted CI. Local packaging remains full
        # by default, and the manifest records this validation boundary.
        $arguments += @('-LE', 'native')
    }
    return ,$arguments
}
