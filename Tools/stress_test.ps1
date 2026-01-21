# NextKey Bug Hunter - Dirty State Stress Test
# Purpose: Break PARTIAL REPLACE, BACKSPACE, and WORD BOUNDARY
# This is NOT a clean stress test - it's designed to dirty the state!
#
# HOW TO RUN:
# 1. Open PowerShell in the project root
# 2. Run: powershell -ExecutionPolicy Bypass -File .\Tools\stress_test.ps1
#
# Prerequisite: Ensure NextKey is running in Debug mode (VS2022 F5)

param(
    [string]$ExeDir = "C:\OpenKey\Sources\OpenKey\win32\OpenKey\x64\Debug",
    [int]$MaxIterations = 10000
)

Add-Type -AssemblyName System.Windows.Forms

function Send-Key($k) {
    [System.Windows.Forms.SendKeys]::SendWait($k)
}

function Rand-Sleep($min, $max) {
    Start-Sleep -Milliseconds (Get-Random -Minimum $min -Maximum $max)
}

function Type-Word($word) {
    foreach ($c in $word.ToCharArray()) {
        $sendChar = $c
        if ($c -eq '+' -or $c -eq '^' -or $c -eq '%' -or $c -eq '~' -or $c -eq '(' -or $c -eq ')' -or $c -eq '[' -or $c -eq ']' -or $c -eq '{' -or $c -eq '}') {
            $sendChar = "{$c}"
        }
        Send-Key($sendChar)
        Rand-Sleep 0 8
    }
}

# Poison sequences - dirty the state before real patterns
$poisonSequences = @(
    "asdfg",
    "qwert",
    "xyzj",          # j = tone key at end
    "abcx",          # x = tone key at end  
    "mnop"
)

# Real test patterns
$testPatterns = @(
    "cungx",
    "thuongr",
    "tooir",
    "khongx",
    "cungs",
    "dungf"
)

# Partial patterns - no tone, let engine handle incomplete state
$partialPatterns = @(
    "cu",
    "cung",
    "thuo",
    "toi",
    "kho"
)

$snapshotPath = Join-Path $ExeDir "debug\debug_snapshot.json"

Write-Host "=== NextKey BUG HUNTER ===" -ForegroundColor Red
Write-Host "This script BREAKS things on purpose!" -ForegroundColor Yellow
Write-Host ""
Write-Host "Techniques used:" -ForegroundColor Cyan
Write-Host "  - 40% NO space at word end"
Write-Host "  - 20% BACKSPACE mid-word"
Write-Host "  - 5% CURSOR move in word"
Write-Host "  - Poison sequences before patterns"
Write-Host "  - Partial word patterns"
Write-Host ""
Write-Host "Watching: $snapshotPath"
Write-Host ""
Read-Host "Press Enter, then CLICK on Notepad++"

# Countdown
Write-Host "CLICK ON NOTEPAD++ NOW!" -ForegroundColor Red
for ($i = 5; $i -gt 0; $i--) {
    Write-Host "  $i..." -ForegroundColor Yellow
    Start-Sleep -Seconds 1
}
Write-Host "  GO!" -ForegroundColor Green

$startTime = Get-Date
$iteration = 0

try {
    while ($iteration -lt $MaxIterations) {
        # Check snapshot
        if (Test-Path $snapshotPath) {
            Write-Host "`n>>> SNAPSHOT DETECTED! <<<" -ForegroundColor Green
            break
        }
        
        $r = Get-Random -Minimum 0 -Maximum 100
        
        # 30% - Inject POISON sequence first
        if ($r -lt 30) {
            $poison = $poisonSequences | Get-Random
            Type-Word $poison
            
            # 50% poison ends with backspace
            if ((Get-Random -Min 0 -Max 100) -lt 50) {
                Send-Key("{BACKSPACE}")
                Rand-Sleep 1 5
            }
            Send-Key(" ")
            Rand-Sleep 3 10
        }
        
        # Pick pattern type
        $patternType = Get-Random -Minimum 0 -Maximum 100
        
        if ($patternType -lt 20) {
            # PARTIAL pattern - no complete tone
            $pattern = $partialPatterns | Get-Random
            Type-Word $pattern
        }
        else {
            # FULL pattern
            $pattern = $testPatterns | Get-Random
            Type-Word $pattern
        }
        
        # === DIRTY TRICKS ===
        
        # 20% - BACKSPACE mid-word (CRITICAL for BS=2,Chars=1 bug)
        if ((Get-Random -Min 0 -Max 100) -lt 20) {
            Send-Key("{BACKSPACE}")
            Rand-Sleep 0 5
            # 50% - retype last char
            if ((Get-Random -Min 0 -Max 100) -lt 50) {
                Send-Key("x")  # Force a tone
                Rand-Sleep 0 3
            }
        }
        
        # 5% - CURSOR move (breaks TypingWord position)
        if ((Get-Random -Min 0 -Max 100) -lt 5) {
            Send-Key("{LEFT}")
            Rand-Sleep 1 3
            Send-Key("{RIGHT}")
            Rand-Sleep 1 3
        }
        
        # 3% - Alt+Tab round trip
        if ((Get-Random -Min 0 -Max 100) -lt 3) {
            Send-Key("%{TAB}")
            Rand-Sleep 40 80
            Send-Key("%{TAB}")
            Rand-Sleep 40 80
        }
        
        # === WORD END ===
        
        # 40% - NO SPACE at end (force engine to handle incomplete)
        if ((Get-Random -Min 0 -Max 100) -lt 60) {
            Send-Key(" ")
        }
        # else: no space, next pattern starts immediately!
        
        Rand-Sleep 3 12
        
        $iteration++
        
        if ($iteration % 200 -eq 0) {
            Write-Host "[$iteration] Testing dirty states..." -ForegroundColor Gray
        }
        
        if ($iteration % 1000 -eq 0) {
            Send-Key("{ENTER}")
        }
    }
}
catch {
    Write-Host "`nInterrupted" -ForegroundColor Yellow
}
finally {
    $duration = ((Get-Date) - $startTime).TotalSeconds
    Write-Host ""
    Write-Host "=== Done ===" -ForegroundColor Green
    Write-Host "Iterations: $iteration | Time: $([math]::Round($duration, 1))s"
    
    if (Test-Path $snapshotPath) {
        Write-Host ""
        Write-Host "SNAPSHOT FOUND!" -ForegroundColor Green
        Get-Content $snapshotPath
    }
    else {
        Write-Host "No snapshot. Check log for TRIPWIRE entries." -ForegroundColor Yellow
    }
}
