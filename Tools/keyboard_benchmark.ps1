<#
.SYNOPSIS
    Vietnamese Keyboard Engine Benchmark (PowerShell)
    Compare typing speed between UniKey, EVKey, NextKey

.DESCRIPTION
    Benchmark script for Vietnamese keyboard engines.
    No installation required - runs directly on Windows PowerShell.

.EXAMPLE
    .\keyboard_benchmark.ps1 -Engine "NextKey" -Text "medium" -Runs 3
    
.EXAMPLE
    .\keyboard_benchmark.ps1 -Engine "UniKey" -Text "long" -Runs 5

.NOTES
    Author: NextKey Team
    Requires: Windows PowerShell 5.0+ or PowerShell Core 7+
#>

param(
    [Parameter(Mandatory = $false)]
    [ValidateSet("nextkey", "unikey", "evkey", "unknown")]
    [string]$Engine = "unknown",
    
    [Parameter(Mandatory = $false)]
    [ValidateSet("short", "medium", "long", "special")]
    [string]$Text = "medium",
    
    [Parameter(Mandatory = $false)]
    [int]$Runs = 3,
    
    [Parameter(Mandatory = $false)]
    [int]$DelayMs = 10,
    
    [Parameter(Mandatory = $false)]
    [switch]$ListTexts,
    
    [Parameter(Mandatory = $false)]
    [switch]$Save
)

# ============================================================
# Benchmark texts - Vietnamese Telex input
# ============================================================
$BenchmarkTexts = @{
    "short"   = @{
        Telex       = "xin chaof banf ddangg laof gix "
        Description = "Short sentence (31 chars)"
    }
    "medium"  = @{
        Telex       = "Buooir sangs hoom nay troiwf ddepj quas. Tooi ddi daoj mootj vongf quanh coong vieen vaf thaays nhuwxng boong hoa nowr rooj. Muaf xuaan ddax ddeens rooif. "
        Description = "Medium paragraph (162 chars)"
    }
    "long"    = @{
        Telex       = "Vieejt Nam laf mootj ddaats nuwowcs coosf kinhsr nhieefuf traams nams lichj suwrt. Tuwf thowif Hunfg Vuwowng ddeeens nafy, daans tooojc vieejt ddax duwngj leebn mootj neefn vans hoaas ddoojc ddaaos. Quees huwowng toois coosf nhuwxng ddoofngs luasf baast ngaats, nhuwxng doofng soongs hieeefn hoaaf, vaaf nhuwxng nguwowif con gasnf guix viwsf langj. Toois yeebu queeb huwowng toois, nowi ddax sinh ra vaaf nuwois duwowxng toois. "
        Description = "Long paragraph about Vietnam (420 chars)"
    }
    "special" = @{
        Telex       = "Aawn aws awj awx awr Aan aas aaf aax aar Een ees eef eex eer Oon oos oof oox oor Own ows owf owx owr Uwn uws uwf uwx uwr dda dde ddi ddo ddu "
        Description = "Special Vietnamese chars test (140 chars)"
    }
}

# ============================================================
# Functions
# ============================================================

function Show-Texts {
    Write-Host "`nAvailable texts:" -ForegroundColor Cyan
    foreach ($key in $BenchmarkTexts.Keys) {
        $data = $BenchmarkTexts[$key]
        Write-Host "`n  $key :" -ForegroundColor Yellow
        Write-Host "    $($data.Description)"
        $preview = if ($data.Telex.Length -gt 60) { $data.Telex.Substring(0, 60) + "..." } else { $data.Telex }
        Write-Host "    Preview: $preview" -ForegroundColor DarkGray
    }
    Write-Host ""
}

function Show-Countdown {
    param([int]$Seconds)
    
    Write-Host "`nPreparing in $Seconds seconds..." -ForegroundColor Yellow
    Write-Host "   Click on target app window (Notepad, VSCode...)" -ForegroundColor Cyan
    
    for ($i = $Seconds; $i -gt 0; $i--) {
        Write-Host "   $i..." -NoNewline
        Start-Sleep -Seconds 1
        Write-Host "`r" -NoNewline
    }
    Write-Host "   GO!           " -ForegroundColor Green
}

function Send-Keys {
    param(
        [string]$Text,
        [int]$DelayMs = 10
    )
    
    Add-Type -AssemblyName System.Windows.Forms
    
    $stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
    
    foreach ($char in $Text.ToCharArray()) {
        # Escape special SendKeys characters
        $sendChar = $char
        if ($char -eq '+' -or $char -eq '^' -or $char -eq '%' -or $char -eq '~' -or $char -eq '(' -or $char -eq ')' -or $char -eq '[' -or $char -eq ']' -or $char -eq '{' -or $char -eq '}') {
            $sendChar = "{$char}"
        }
        [System.Windows.Forms.SendKeys]::SendWait($sendChar)
        Start-Sleep -Milliseconds $DelayMs
    }
    
    $stopwatch.Stop()
    return $stopwatch.ElapsedMilliseconds
}

function Run-SingleBenchmark {
    param(
        [string]$Text,
        [int]$DelayMs
    )
    
    $charCount = $Text.Length
    
    # Wait for focus
    Start-Sleep -Milliseconds 100
    
    # Measure time
    $elapsedMs = Send-Keys -Text $Text -DelayMs $DelayMs
    
    # Calculate metrics
    $charsPerSecond = if ($elapsedMs -gt 0) { ($charCount / $elapsedMs) * 1000 } else { 0 }
    $avgLatencyMs = if ($charCount -gt 0) { $elapsedMs / $charCount } else { 0 }
    
    return @{
        CharCount      = $charCount
        TotalTimeMs    = $elapsedMs
        CharsPerSecond = [math]::Round($charsPerSecond, 1)
        AvgLatencyMs   = [math]::Round($avgLatencyMs, 3)
    }
}

function Show-Summary {
    param(
        [array]$Results,
        [string]$EngineName
    )
    
    if ($Results.Count -eq 0) { return }
    
    $times = $Results | ForEach-Object { $_.TotalTimeMs }
    $speeds = $Results | ForEach-Object { $_.CharsPerSecond }
    
    $avgTime = ($times | Measure-Object -Average).Average
    $minTime = ($times | Measure-Object -Minimum).Minimum
    $maxTime = ($times | Measure-Object -Maximum).Maximum
    
    $avgSpeed = ($speeds | Measure-Object -Average).Average
    $minSpeed = ($speeds | Measure-Object -Minimum).Minimum
    $maxSpeed = ($speeds | Measure-Object -Maximum).Maximum
    
    Write-Host "`n" -NoNewline
    Write-Host ("=" * 60) -ForegroundColor Cyan
    Write-Host "BENCHMARK RESULTS - $($EngineName.ToUpper())" -ForegroundColor Cyan
    Write-Host ("=" * 60) -ForegroundColor Cyan
    Write-Host "  Runs: $($Results.Count)"
    Write-Host "  Chars per run: $($Results[0].CharCount)"
    Write-Host ""
    Write-Host "  Total Time:" -ForegroundColor Yellow
    Write-Host "      Min: $([math]::Round($minTime, 2)) ms"
    Write-Host "      Max: $([math]::Round($maxTime, 2)) ms"
    Write-Host "      Avg: $([math]::Round($avgTime, 2)) ms" -ForegroundColor Green
    Write-Host ""
    Write-Host "  Speed (chars/second):" -ForegroundColor Yellow
    Write-Host "      Min: $([math]::Round($minSpeed, 1))"
    Write-Host "      Max: $([math]::Round($maxSpeed, 1))"
    Write-Host "      Avg: $([math]::Round($avgSpeed, 1))" -ForegroundColor Green
    Write-Host ("=" * 60) -ForegroundColor Cyan
}

function Save-Results {
    param(
        [array]$Results,
        [string]$EngineName
    )
    
    $timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
    $filename = "benchmark_${EngineName}_${timestamp}.txt"
    
    $times = $Results | ForEach-Object { $_.TotalTimeMs }
    $speeds = $Results | ForEach-Object { $_.CharsPerSecond }
    
    $content = @"
Benchmark Results - $EngineName
Timestamp: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
============================================

Runs: $($Results.Count)
Chars per run: $($Results[0].CharCount)
Delay per char: $DelayMs ms

Total time (avg): $([math]::Round(($times | Measure-Object -Average).Average, 2)) ms
Speed (avg): $([math]::Round(($speeds | Measure-Object -Average).Average, 1)) chars/s

Individual runs:
$($Results | ForEach-Object { "  Run: $($_.TotalTimeMs) ms, $($_.CharsPerSecond) chars/s" } | Out-String)
"@
    
    $content | Out-File -FilePath $filename -Encoding UTF8
    Write-Host "`nSaved results to: $filename" -ForegroundColor Green
}

# ============================================================
# Main
# ============================================================

# List texts
if ($ListTexts) {
    Show-Texts
    exit 0
}

# Check if text exists
if (-not $BenchmarkTexts.ContainsKey($Text)) {
    Write-Host "ERROR: Text '$Text' not found" -ForegroundColor Red
    Show-Texts
    exit 1
}

$textData = $BenchmarkTexts[$Text]
$telexInput = $textData.Telex

# Banner
Write-Host ""
Write-Host ("=" * 60) -ForegroundColor Cyan
Write-Host "VIETNAMESE KEYBOARD ENGINE BENCHMARK" -ForegroundColor Cyan
Write-Host ("=" * 60) -ForegroundColor Cyan
Write-Host "  Engine: $($Engine.ToUpper())" -ForegroundColor Yellow
Write-Host "  Text: $Text ($($textData.Description))"
Write-Host "  Runs: $Runs"
Write-Host "  Delay: ${DelayMs}ms per char"
Write-Host ""
Write-Host "IMPORTANT:" -ForegroundColor Yellow
Write-Host "  1. Make sure Vietnamese keyboard engine is ON" -ForegroundColor White
Write-Host "  2. Click on target app before countdown ends"
Write-Host "  3. DO NOT move mouse or type during test"
Write-Host ""

# Confirm
Read-Host "Press Enter to start"

# Run benchmark
$results = @()

for ($i = 1; $i -le $Runs; $i++) {
    Write-Host "`n--- Run $i/$Runs ---" -ForegroundColor Magenta
    
    if ($i -eq 1) {
        Show-Countdown -Seconds 5
    }
    else {
        Write-Host "   Running..." -ForegroundColor DarkGray
        Start-Sleep -Seconds 1
    }
    
    $result = Run-SingleBenchmark -Text $telexInput -DelayMs $DelayMs
    $results += $result
    
    Write-Host "   Done: $($result.TotalTimeMs) ms" -ForegroundColor Green
    Write-Host "   Speed: $($result.CharsPerSecond) chars/s"
    
    # Newline after each run
    Add-Type -AssemblyName System.Windows.Forms
    [System.Windows.Forms.SendKeys]::SendWait("{ENTER}{ENTER}")
    Start-Sleep -Milliseconds 500
}

# Show results
Show-Summary -Results $results -EngineName $Engine

# Save if flag set
if ($Save) {
    Save-Results -Results $results -EngineName $Engine
}

Write-Host "`nBenchmark complete!" -ForegroundColor Green
Write-Host "   Run again with a different engine to compare.`n"
