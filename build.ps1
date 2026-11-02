<#
.SYNOPSIS
  Builds Rainy Windowpane on Windows using the Pebble SDK in a container.

.EXAMPLE
  ./build.ps1                       # build build/RainyWindowpane.pbw
  ./build.ps1 -Phone 192.168.1.23   # build, then install via developer connection
  ./build.ps1 -Zip                  # also write dist/Rainy-Windowpane-cloudpebble.zip
  ./build.ps1 -Shell                # open a shell with the `pebble` tool
#>
param(
    [string]$Phone,
    [switch]$Zip,
    [switch]$Shell
)
function Test-Engine([string]$name) {
    if (-not (Get-Command $name -ErrorAction SilentlyContinue)) { return $false }
    & $name info *> $null
    return ($LASTEXITCODE -eq 0)
}
$engine = if (Test-Engine 'docker') { 'docker' }
          elseif (Test-Engine 'podman') { 'podman' }
          else { throw 'Install Docker Desktop or Podman Desktop first (engine must be running).' }

$root = $PSScriptRoot
$image = 'pebble-sdk'
$volume = 'pebble-sdk'

& $engine image inspect $image *> $null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Building the $image image (first run only)..."
    & $engine build -t $image (Join-Path $root 'build-env')
    if ($LASTEXITCODE -ne 0) { throw 'Image build failed.' }
}

# Mount path basename becomes the .pbw name (build/RainyWindowpane.pbw).
$run = @('run', '--rm', '-v', "${root}:/RainyWindowpane", '-w', '/RainyWindowpane', '-v', "${volume}:/root/.pebble-sdk")
$setup = 'pebble sdk list 2>/dev/null | grep -q active || pebble sdk install latest'

if ($Shell) {
    & $engine @run -it $image bash -c "$setup; exec bash"
    exit $LASTEXITCODE
}

$cmd = "$setup && pebble build"
if ($Phone) { $cmd += " && pebble install --phone $Phone" }
& $engine @run $image bash -c $cmd
if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
Write-Host "Built $(Join-Path $root 'build\RainyWindowpane.pbw')"

if ($Zip) {
    python (Join-Path $root 'build-env\make_zip.py') $root
}
