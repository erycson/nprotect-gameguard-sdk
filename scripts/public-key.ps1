param([string]$InputKey, [Parameter(Mandatory=$true)][string]$Output)
$ErrorActionPreference = 'Stop'
if (-not $InputKey) {
    $source = '#define GG_PUBLIC_KEY_AVAILABLE 0' + "`n" + 'static const unsigned char gg_public_key_blob[] = {0};' + "`n"
} else {
    $rsa = [System.Security.Cryptography.RSA]::Create()
    try {
        $rsa.ImportFromPem([IO.File]::ReadAllText((Resolve-Path -LiteralPath $InputKey)))
        $p = $rsa.ExportParameters($false)
        if ($p.Modulus.Length -ne 64 -or $p.Exponent.Length -gt 4) { throw 'A chave publica deve ser RSA de 512 bits.' }
        $exponent = [uint32]0
        foreach ($b in $p.Exponent) { $exponent = ($exponent -shl 8) -bor $b }
        $modulus = [byte[]]$p.Modulus.Clone(); [Array]::Reverse($modulus)
        $blob = [byte[]]@(6,2,0,0,0,36,0,0,82,83,65,49,0,2,0,0) + [BitConverter]::GetBytes($exponent) + $modulus
        $hex = ($blob | ForEach-Object { '0x{0:x2}' -f $_ }) -join ','
        $source = '#define GG_PUBLIC_KEY_AVAILABLE 1' + "`n" + 'static const unsigned char gg_public_key_blob[] = {' + $hex + '};' + "`n"
    } finally { $rsa.Dispose() }
}
[IO.File]::WriteAllText([IO.Path]::GetFullPath($Output), $source, [Text.Encoding]::ASCII)
