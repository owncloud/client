$COLOR=$Args[0]
Write-Host "Color:" $COLOR
if (-not $COLOR) {
    Write-Host "Please specify color"
    exit(1)
}
Get-ChildItem "*.svg" | % {
    $tmp = Get-Content $_
    $tmp = $tmp -replace "fill=`"\S+`"", "fill=`"$COLOR`""
    Set-Content -Path $_ -Value $tmp

}
