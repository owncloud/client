Param
(
  [Parameter(Mandatory=$true)]
  [string]$ProductName
)

# https://github.com/MicrosoftDocs/winrt-api/issues/1130
$SyncRootManager = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\SyncRootManager"
$GetRootItem = Get-Item $SyncRootManager
Get-ChildItem -Path  $SyncRootManager | ForEach-Object {
    # legacy clients always used that prefix owncloud_

    $name = $_.Name.Substring($GetRootItem.Name.Length + 1)
    if ($name.StartsWith("OC-TEST", "CurrentCultureIgnoreCase") -or $name.StartsWith("owncloud_", "CurrentCultureIgnoreCase") -or $name.StartsWith("$ProductName", "CurrentCultureIgnoreCase")) {
        Write-Host $_
        Remove-Item -Recurse $_.PsPath 
    }
}

Get-ChildItem -Path "HKCU:\Software\Classes\CLSID\"  | ForEach-Object {
    $key = (get-itemproperty $_.PsPath)."(default)"
    if ($key) {
        # unit tests (OC-TEST) leave stuff behind
        if ($key.StartsWith("OC-TEST", "CurrentCultureIgnoreCase") -or $key.StartsWith("$ProductName", "CurrentCultureIgnoreCase")) {
            Write-Host $key, $_
            Remove-Item -Recurse $_.PsPath
        }
    }
}
Get-Process explorer | Stop-Process
Pause
