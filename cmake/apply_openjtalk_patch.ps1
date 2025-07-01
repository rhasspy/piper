param([string]$SourceDir)

$filePath = Join-Path $SourceDir "mecab/src/dictionary.cpp"
Write-Host "Checking file: $filePath"

if (Test-Path $filePath) {
    $content = Get-Content -Path $filePath -Raw
    
    if (-not ($content -match '__cplusplus.*201703L')) {
        Write-Host "Applying C++17 compatibility patch..."
        
        # Apply the patch - the template is not inside a namespace
        $content = $content -replace `
            '(template\s*<typename\s+T1,\s*typename\s+T2>\s*[\r\n]+)(struct\s+pair_1st_cmp:\s*public\s+std::binary_function<bool,\s*T1,\s*T2>\s*\{)', `
            '$1#if __cplusplus >= 201703L`r`nstruct pair_1st_cmp {`r`n  typedef T1 first_argument_type;`r`n  typedef T2 second_argument_type;`r`n  typedef bool result_type;`r`n#else`r`n$2`r`n#endif'
        
        Set-Content -Path $filePath -Value $content -NoNewline
        Write-Host "Successfully applied C++17 compatibility patch to dictionary.cpp"
    } else {
        Write-Host "File already patched"
    }
} else {
    Write-Host "ERROR: File not found: $filePath"
    exit 1
}