param([string]$SourceDir)

$filePath = Join-Path $SourceDir "mecab/src/dictionary.cpp"
Write-Host "Checking file: $filePath"

if (Test-Path $filePath) {
    $content = Get-Content -Path $filePath -Raw
    
    if (-not ($content -match '__cplusplus.*201703L')) {
        Write-Host "Applying C++17 compatibility patch..."
        
        # More robust approach - replace the struct definition entirely
        $oldPattern = 'template\s*<typename\s+T1,\s*typename\s+T2>\s*\r?\n\s*struct\s+pair_1st_cmp:\s*public\s+std::binary_function<bool,\s*T1,\s*T2>\s*\{'
        $newContent = @"
template <typename T1, typename T2>
#if __cplusplus >= 201703L
struct pair_1st_cmp {
  typedef T1 first_argument_type;
  typedef T2 second_argument_type;
  typedef bool result_type;
#else
struct pair_1st_cmp: public std::binary_function<bool, T1, T2> {
#endif
"@
        
        if ($content -match $oldPattern) {
            $content = $content -replace $oldPattern, $newContent
            Set-Content -Path $filePath -Value $content -NoNewline
            Write-Host "Successfully applied C++17 compatibility patch to dictionary.cpp"
        } else {
            Write-Host "WARNING: Could not find expected pattern to patch"
            Write-Host "Attempting alternative patch method..."
            
            # Alternative: Simply remove binary_function inheritance
            $content = $content -replace ': public std::binary_function<bool, T1, T2>', ''
            Set-Content -Path $filePath -Value $content -NoNewline
            Write-Host "Applied alternative C++17 compatibility patch (removed binary_function)"
        }
    } else {
        Write-Host "File already patched"
    }
} else {
    Write-Host "ERROR: File not found: $filePath"
    exit 1
}