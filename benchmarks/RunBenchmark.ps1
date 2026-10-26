
$ErrorActionPreference = "Stop"

$root = (Resolve-Path "$PSScriptRoot\..").Path

$exe = Join-Path $root "build-release\fastindex_cli.exe"
$docs = Join-Path $root "build\benchmark_docs"
$outDir = Join-Path $root "build-release"
$csv = Join-Path $outDir "benchmark_results.csv"

if (-not (Test-Path $exe)) {
    throw "Release executable not found: $exe"
}

if (-not (Test-Path $docs)) {
    throw "Benchmark dataset not found: $docs"
}

$fileCount = @(
    Get-ChildItem $docs -Filter "*.txt" -File
).Count

Write-Host "FastIndex Release Benchmark"
Write-Host "Dataset: $fileCount files"

$modes = @(
    @{ Name = "Sequential"; Workers = 0 },
    @{ Name = "Parallel1";  Workers = 1 },
    @{ Name = "Parallel2";  Workers = 2 },
    @{ Name = "Parallel4";  Workers = 4 },
    @{ Name = "Parallel8";  Workers = 8 }
)

$results = @()

function Invoke-Case {
    param(
        [hashtable]$Mode,
        [int]$Round,
        [bool]$Record
    )

    $db = Join-Path $outDir (
        "benchmark_" + $Mode.Name + ".db"
    )

    # Always use a fresh database.
    foreach ($suffix in @("", "-journal", "-wal", "-shm")) {
        Remove-Item ($db + $suffix) `
            -Force -ErrorAction SilentlyContinue
    }

    $cliArgs = @("index", $docs, $db)

    if ($Mode.Workers -gt 0) {
        $cliArgs += @(
            "--parallel",
            [string]$Mode.Workers
        )
    }

    $output = & $exe @cliArgs
    $exitCode = $LASTEXITCODE

    $text = $output -join "`n"

    if ($exitCode -ne 0) {
        throw "Indexing failed: $text"
    }

    $match = [regex]::Match(
        $text,
        "Duration:\s*([0-9]+(?:\.[0-9]+)?)\s*ms"
    )

    if (-not $match.Success) {
        throw "Duration not found: $text"
    }

    $indexed = [regex]::Match(
        $text, "Indexed:\s*(\d+)"
    )

    $skipped = [regex]::Match(
        $text, "Skipped:\s*(\d+)"
    )

    if (
        -not $indexed.Success -or
        -not $skipped.Success -or
        [int]$indexed.Groups[1].Value -ne $fileCount -or
        [int]$skipped.Groups[1].Value -ne 0
    ) {
        throw "Unexpected indexing results: $text"
    }

    $duration = [double]::Parse(
        $match.Groups[1].Value,
        [Globalization.CultureInfo]::InvariantCulture
    )

    if ($Record) {
        $script:results += [PSCustomObject]@{
            Mode = $Mode.Name
            Run = $Round
            DurationMs = $duration
        }
    }

    $label = if ($Record) { "Run $Round" } else { "Warmup" }

    Write-Host (
        "{0}: {1} = {2:N2} ms" -f
        $label, $Mode.Name, $duration
    )
}

Write-Host "`n--- Warmup ---"

foreach ($mode in $modes) {
    Invoke-Case -Mode $mode -Round 0 -Record $false
}

Write-Host "`n--- Measurements ---"

for ($round = 1; $round -le 9; $round++) {
    # Rotate execution order between rounds.
    for ($i = 0; $i -lt $modes.Count; $i++) {
        $index = ($i + $round - 1) % $modes.Count

        Invoke-Case `
            -Mode $modes[$index] `
            -Round $round `
            -Record $true
    }
}

$results | Export-Csv `
    -Path $csv `
    -NoTypeInformation `
    -Encoding UTF8

Write-Host "`n--- Median Results ---"

$baseline = 0

foreach ($mode in $modes) {
    $values = @(
        $results |
            Where-Object { $_.Mode -eq $mode.Name } |
            Sort-Object DurationMs |
            Select-Object -ExpandProperty DurationMs
    )

    $median = $values[4]

    if ($mode.Name -eq "Sequential") {
        $baseline = $median
    }

    $speedup = $baseline / $median

    Write-Host (
        "{0}: {1:N2} ms | {2:N2}x" -f
        $mode.Name, $median, $speedup
    )
}

Write-Host "`nResults saved to:"
Write-Host $csv
