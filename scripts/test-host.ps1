param([string]$Compiler = 'gcc', [string]$GccLauncher = '')
$ErrorActionPreference = 'Stop'
$taskRepo = Split-Path $PSScriptRoot -Parent
$taskBuild = Join-Path $taskRepo 'build\host'
New-Item -ItemType Directory -Path $taskBuild -Force | Out-Null
Copy-Item -Path (Join-Path $taskRepo 'src\*') -Destination $taskBuild -Force
Push-Location $taskBuild
try {
    foreach ($taskTest in @('QA','MEMQA','UIQA')) {
        $taskArgs = @('-x','c','-std=c89','-DHOST_QA=1','-O2','-Wall','-Wextra','-pedantic',"$taskTest.C",'-o',"$taskTest.exe")
        if ($GccLauncher) { & python $GccLauncher -- @taskArgs *> "$taskTest-build.txt" }
        else { & $Compiler @taskArgs *> "$taskTest-build.txt" }
        if ($LASTEXITCODE) { Get-Content "$taskTest-build.txt"; throw "$taskTest compile failed" }
        & ".\$taskTest.exe" *> "$taskTest-test.txt"
        if ($LASTEXITCODE) { Get-Content "$taskTest-test.txt"; throw "$taskTest tests failed" }
        Get-Content "$taskTest-test.txt"
    }
} finally { Pop-Location }
