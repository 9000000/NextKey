# NextKey Stress Test Script v2.0
# Tests clipboard-based Vietnamese input for race conditions and character issues
# Run: Right-click -> "Run with PowerShell" OR open PowerShell and run: .\fast_type_test.ps1

Add-Type -AssemblyName System.Windows.Forms

# Vietnamese test sentences
$testSentences = @{
    "short"   = "Việt Nam đẹp lắm"
    "medium"  = "Hôm nay trời đẹp quá, tôi muốn đi chơi công viên."
    "long"    = "Cộng hòa xã hội chủ nghĩa Việt Nam độc lập tự do hạnh phúc"
    "complex" = "Một nước có độ lệch lớn giữa mức sống tầng lớp thượng lưu và tầng lớp bình dân"
    "mixed"   = "OpenKey là phần mềm gõ tiếng Việt mã nguồn mở, miễn phí 100%"
}

function Start-StressTest {
    param(
        [string]$Sentence,
        [int]$Iterations = 10,
        [int]$DelayBetweenCharsMs = 0,
        [int]$DelayBetweenWordsMs = 50,
        [int]$DelayBetweenIterationsMs = 500
    )
    
    Write-Host ""
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "STRESS TEST CONFIGURATION" -ForegroundColor Cyan
    Write-Host "========================================" -ForegroundColor Cyan
    Write-Host "Sentence: $Sentence"
    Write-Host "Iterations: $Iterations"
    Write-Host "Char delay: ${DelayBetweenCharsMs}ms | Word delay: ${DelayBetweenWordsMs}ms"
    Write-Host ""
    Write-Host ">>> Focus target window in 5 seconds... <<<" -ForegroundColor Yellow
    Start-Sleep -Seconds 5
    
    for ($i = 1; $i -le $Iterations; $i++) {
        Write-Host "Iteration $i / $Iterations" -ForegroundColor Green
        
        $words = $Sentence -split '\s+'
        foreach ($word in $words) {
            foreach ($char in $word.ToCharArray()) {
                [System.Windows.Forms.SendKeys]::SendWait([char]$char)
                if ($DelayBetweenCharsMs -gt 0) {
                    Start-Sleep -Milliseconds $DelayBetweenCharsMs
                }
            }
            [System.Windows.Forms.SendKeys]::SendWait(' ')
            if ($DelayBetweenWordsMs -gt 0) {
                Start-Sleep -Milliseconds $DelayBetweenWordsMs
            }
        }
        
        [System.Windows.Forms.SendKeys]::SendWait('{ENTER}')
        
        if ($DelayBetweenIterationsMs -gt 0) {
            Start-Sleep -Milliseconds $DelayBetweenIterationsMs
        }
    }
    
    Write-Host ""
    Write-Host "Test complete!" -ForegroundColor Green
}

function Show-Menu {
    Clear-Host
    Write-Host "============================================" -ForegroundColor Magenta
    Write-Host "   NextKey Stress Test Tool v2.0" -ForegroundColor Magenta
    Write-Host "============================================" -ForegroundColor Magenta
    Write-Host ""
    Write-Host "Stress Tests:" -ForegroundColor Yellow
    Write-Host "  1. Short - 10 iterations, ultra-fast"
    Write-Host "  2. Medium - 5 iterations, fast"
    Write-Host "  3. Long (Cong hoa xa hoi...) - 5 iterations"
    Write-Host "  4. Complex paragraph - 3 iterations"
    Write-Host "  5. Mixed Vietnamese - 5 iterations"
    Write-Host ""
    Write-Host "App-Specific Tests:" -ForegroundColor Yellow
    Write-Host "  B. Browser test (slow mode for Firefox)"
    Write-Host "  G. Game mode (zero delays, max stress)"
    Write-Host ""
    Write-Host "Custom:" -ForegroundColor Yellow
    Write-Host "  C. Custom sentence input"
    Write-Host ""
    Write-Host "VNI Quick Tests:" -ForegroundColor Yellow
    Write-Host "  V1. viet61 -> viết"
    Write-Host "  V2. thu71 -> thủ"
    Write-Host "  V3. cuoi61 -> cuối"
    Write-Host ""
    Write-Host "  Q. Quit"
    Write-Host ""
}

# Main loop
while ($true) {
    Show-Menu
    $choice = Read-Host "Enter choice"
    
    switch ($choice.ToUpper()) {
        "1" { Start-StressTest -Sentence $testSentences["short"] -Iterations 10 -DelayBetweenCharsMs 0 -DelayBetweenWordsMs 10 }
        "2" { Start-StressTest -Sentence $testSentences["medium"] -Iterations 5 -DelayBetweenCharsMs 0 -DelayBetweenWordsMs 30 }
        "3" { Start-StressTest -Sentence $testSentences["long"] -Iterations 5 -DelayBetweenCharsMs 0 -DelayBetweenWordsMs 50 }
        "4" { Start-StressTest -Sentence $testSentences["complex"] -Iterations 3 -DelayBetweenCharsMs 5 -DelayBetweenWordsMs 100 }
        "5" { Start-StressTest -Sentence $testSentences["mixed"] -Iterations 5 -DelayBetweenCharsMs 0 -DelayBetweenWordsMs 50 }
        
        "B" { 
            Write-Host "Browser test: Testing with delays to detect race conditions" -ForegroundColor Cyan
            Start-StressTest -Sentence $testSentences["long"] -Iterations 10 -DelayBetweenCharsMs 5 -DelayBetweenWordsMs 100 -DelayBetweenIterationsMs 1000
        }
        
        "G" {
            Write-Host "Game mode: ZERO delays - maximum stress" -ForegroundColor Red
            Start-StressTest -Sentence $testSentences["long"] -Iterations 20 -DelayBetweenCharsMs 0 -DelayBetweenWordsMs 0 -DelayBetweenIterationsMs 100
        }
        
        "C" {
            $customSentence = Read-Host "Enter Vietnamese sentence"
            $iterations = Read-Host "Iterations (default 5)"
            if (-not $iterations) { $iterations = 5 }
            Start-StressTest -Sentence $customSentence -Iterations ([int]$iterations)
        }
        
        "V1" {
            Write-Host "Focus window in 3 seconds..." -ForegroundColor Yellow
            Start-Sleep -Seconds 3
            [System.Windows.Forms.SendKeys]::SendWait("viet61")
        }
        "V2" {
            Write-Host "Focus window in 3 seconds..." -ForegroundColor Yellow
            Start-Sleep -Seconds 3
            [System.Windows.Forms.SendKeys]::SendWait("thu71")
        }
        "V3" {
            Write-Host "Focus window in 3 seconds..." -ForegroundColor Yellow
            Start-Sleep -Seconds 3
            [System.Windows.Forms.SendKeys]::SendWait("cuoi61")
        }
        
        "Q" { Write-Host "Goodbye!"; exit }
        default { Write-Host "Invalid choice" -ForegroundColor Red }
    }
    
    Write-Host ""
    Write-Host "Press Enter to continue..."
    Read-Host
}
