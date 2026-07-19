$htmlContent = Get-Content -Raw -Encoding UTF8 index.html
$headerContent = @"
#pragma once

const char index_html[] PROGMEM = R"rawliteral(
$htmlContent
)rawliteral";

"@

# UTF8 with BOM (Arduino IDE compatibility)
[System.IO.File]::WriteAllText("$PSScriptRoot\index_html.h", $headerContent, [System.Text.Encoding]::UTF8)
Write-Host "index_html.h has been generated successfully!"
