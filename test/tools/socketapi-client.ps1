param(
    [string]$message
)

if (-not $message -or $message -eq "--help") {
    Write-Host "Usage: .\socketapi-client.ps1 -message <message>"
    Write-Host "Sends a SocketAPI message to the ownCloud client application."
    Write-Host "Examples:"
    Write-Host "    .\socketapi-client.ps1 'VERSION'"
    Write-Host "    .\socketapi-client.ps1 'GET_STRINGS'"
    Write-Host "    .\socketapi-client.ps1 'DELETE_ITEM /path/to/item'"
    exit
}

# Define the pipe name and server
$serverName = "."  # Local machine
$pipeName = "ownCloud-" + $env:USERNAME

# Create the named pipe client stream
$pipe = New-Object System.IO.Pipes.NamedPipeClientStream($serverName, $pipeName, [System.IO.Pipes.PipeDirection]::InOut)

try {
    # Connect to the pipe (timeout after 5 seconds)
    $pipe.Connect(5000)
    if (-not $pipe.IsConnected) { throw "Pipe not connected." }
    # Convert the string to bytes
    $msgToSend = $message + "`n"
    $bytes = [System.Text.Encoding]::UTF8.GetBytes($msgToSend)

    # Write the bytes to the pipe
    $pipe.Write($bytes, 0, $bytes.Length)
    $pipe.Flush()
    Write-Host "Message sent to named pipe: $pipeName"

    $pipe.Dispose()
}
catch {
    Write-Error "Failed to send message: $_"
}
