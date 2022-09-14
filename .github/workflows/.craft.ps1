if ($IsWindows) {
    $python = (Get-Command py).Source
} else {
    $python = (Get-Command python3).Source
}
$command = @("${env:GITHUB_WORKSPACE}/craft/CraftMaster/CraftMaster/CraftMaster.py",
             "--config", "${env:GITHUB_WORKSPACE}/.craft.ini",
             "--config-override", "${env:GITHUB_WORKSPACE}/.github/workflows/craft_override.ini",
             "--target", "${env:CRAFT_TARGET}",
             "--variables", "WORKSPACE=${env:GITHUB_WORKSPACE}/craft") + $args

Write-Host "Exec: ${python} ${command}"

& $python @command
