$FileContents = Get-Content -Raw version.txt

$VersionNumber=$null

foreach($line in ($FileContents -split [System.Environment]::NewLine))
{
     $VersionNumber += $Line.Split(' ')[-1].Trim() + "."
}

$VersionNumber = $VersionNumber.TrimEnd('.')

return $VersionNumber