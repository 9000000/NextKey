# NextKey Fast Typing Test Script
# This script simulates fast typing to test clipboard-based input issues
# Run: Right-click -> "Run with PowerShell" OR open PowerShell and run: .\fast_type_test.ps1

Add-Type -AssemblyName System.Windows.Forms

function Send-FastKeys {
    param(
        [string]$Keys,
        [int]$DelayMs = 0
    )
    
    Write-Host "Sending keys: $Keys (delay: ${DelayMs}ms between keys)"
    Write-Host "Focus the target window (Notepad, Firefox, etc.) within 3 seconds..."
    Start-Sleep -Seconds 3
    
    foreach ($char in $Keys.ToCharArray()) {
        [System.Windows.Forms.SendKeys]::SendWait($char)
        if ($DelayMs -gt 0) {
            Start-Sleep -Milliseconds $DelayMs
        }
    }
    Write-Host "Done!"
}

function Show-Menu {
    Clear-Host
    Write-Host "=========================================="
    Write-Host "   NextKey Fast Typing Test Tool"
    Write-Host "=========================================="
    Write-Host ""
    Write-Host "VNI Test Cases (gõ nhanh):"
    Write-Host "  1. viet61 -> viết (test duplicate 'i')"
    Write-Host "  2. thu71  -> thủ  (test character loss)"
    Write-Host "  3. be1    -> bé   (test simple tone)"
    Write-Host "  4. cuoi61 -> cuối (test duplicate 'u')"
    Write-Host "  5. Ma1 em ho62ng xinh xa81n va2 de64 thuo7ng (full sentence)"
    Write-Host ""
    Write-Host "Telex Test Cases:"
    Write-Host "  6. vieestt -> việt"
    Write-Host "  7. thuwr   -> thử"  
    Write-Host "  8. Vieejt Nam -> Việt Nam"
    Write-Host ""
    Write-Host "Speed Control:"
    Write-Host "  F. Ultra Fast (0ms delay)"
    Write-Host "  N. Normal (50ms delay)"
    Write-Host "  S. Slow (100ms delay)"
    Write-Host ""
    Write-Host "  Q. Quit"
    Write-Host ""
}

$delay = 0  # Default: ultra fast

while ($true) {
    Show-Menu
    Write-Host "Current delay: ${delay}ms"
    $choice = Read-Host "Enter choice"
    
    switch ($choice.ToUpper()) {
        "1" { Send-FastKeys "viet61" $delay }
        "2" { Send-FastKeys "thu71" $delay }
        "3" { Send-FastKeys "be1" $delay }
        "4" { Send-FastKeys "cuoi61" $delay }
        "5" { Send-FastKeys "Ma1 em ho62ng xinh xa81n va2 de64 thuo7ng" $delay }
        "6" { Send-FastKeys "vieestt" $delay }
        "7" { Send-FastKeys "thuwr" $delay }
        "8" { Send-FastKeys "Vieejt Nam" $delay }
        "F" { $delay = 0; Write-Host "Set to Ultra Fast (0ms)" }
        "N" { $delay = 50; Write-Host "Set to Normal (50ms)" }
        "S" { $delay = 100; Write-Host "Set to Slow (100ms)" }
        "Q" { Write-Host "Goodbye!"; exit }
        default { Write-Host "Invalid choice. Press Enter to continue..."; Read-Host }
    }
    
    if ($choice -match "^[1-8]$") {
        Write-Host ""
        Write-Host "Press Enter to continue..."
        Read-Host
    }
}
