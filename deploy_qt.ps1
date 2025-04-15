# Qt Deployment Script for MIB-Studio
# This script copies all necessary Qt plugins to the build directory

# Define paths
$buildDir = "build\Release"
$vcpkgDir = "build\vcpkg_installed\x64-windows"
$qtPluginsDir = "$vcpkgDir\Qt6\plugins"

# Create plugin directories
Write-Host "Creating plugin directories..."
mkdir -Force "$buildDir\platforms" | Out-Null
mkdir -Force "$buildDir\imageformats" | Out-Null
mkdir -Force "$buildDir\styles" | Out-Null

# Copy platform plugins
Write-Host "Copying platform plugins..."
if (Test-Path "$qtPluginsDir\platforms") {
    Copy-Item "$qtPluginsDir\platforms\*.dll" -Destination "$buildDir\platforms\" -Force
}

# Copy imageformats plugins
Write-Host "Copying imageformats plugins..."
if (Test-Path "$qtPluginsDir\imageformats") {
    Copy-Item "$qtPluginsDir\imageformats\*.dll" -Destination "$buildDir\imageformats\" -Force
}

# Copy style plugins
Write-Host "Copying style plugins..."
if (Test-Path "$qtPluginsDir\styles") {
    Copy-Item "$qtPluginsDir\styles\*.dll" -Destination "$buildDir\styles\" -Force
}

# Verify deployment
Write-Host "Verifying deployment..."
if (Test-Path "$buildDir\platforms\qwindows.dll") {
    Write-Host "Qt platform plugin deployed successfully." -ForegroundColor Green
} else {
    Write-Host "Qt platform plugin not found! Deployment may be incomplete." -ForegroundColor Red
}

Write-Host "Qt deployment completed." 