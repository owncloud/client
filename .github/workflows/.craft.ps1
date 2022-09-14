if ($IsWindows) {
    $python = (Get-Command py).Source
} else {
    $python = (Get-Command python3).Source
}

if ($null -eq $env:CRAFT_DIR) {
    $env:CRAFT_DIR = "${env:HOME}/craft/CraftMaster/CraftMaster"
}

$command = @("${env:CRAFT_DIR}/CraftMaster.py",
             "--config", "${env:GITHUB_WORKSPACE}/.craft.ini",
             "--config-override", "${env:GITHUB_WORKSPACE}/.github/workflows/craft_override.ini",
             "--target", "${env:CRAFT_TARGET}",
             "--variables", "WORKSPACE=${env:HOME}/craft") + $args

Write-Host "Exec: ${python} ${command}"

& $python @command
