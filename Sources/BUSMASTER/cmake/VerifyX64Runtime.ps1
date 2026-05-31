param(
    [Parameter(Mandatory = $true)]
    [string] $DeployDirectory
)

$ErrorActionPreference = "Stop"
$x64Machine = 0x8664
$invalidDlls = @()

Get-ChildItem -Path $DeployDirectory -Filter *.dll -File -Recurse | ForEach-Object {
    $bytes = [IO.File]::ReadAllBytes($_.FullName)
    if ($bytes.Length -lt 64) {
        $invalidDlls += "$($_.FullName) (not a valid PE file)"
        return
    }

    $peOffset = [BitConverter]::ToInt32($bytes, 0x3c)
    if ($peOffset -lt 0 -or $peOffset + 6 -gt $bytes.Length) {
        $invalidDlls += "$($_.FullName) (not a valid PE file)"
        return
    }

    $machine = [BitConverter]::ToUInt16($bytes, $peOffset + 4)
    if ($machine -ne $x64Machine) {
        $invalidDlls += "$($_.FullName) (PE machine 0x$($machine.ToString('X4')))"
    }
}

if ($invalidDlls.Count -gt 0) {
    throw "Non-x64 DLLs found in the NovoBusAnalyzer runtime folder:`n$($invalidDlls -join "`n")"
}

Write-Host "Verified x64 runtime DLLs in $DeployDirectory"
