# Leer y concatenar el contenido de dos archivos
$contenido = Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\IntegrateAndPredict.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\AssignCells.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\Sort_ExtractBit.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\Sort_BlockScan.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\Sort_AddOffset.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\Sort_Reorder.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\FindCellBounds.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\ComputeLambda.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\ComputeDeltaP.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\ApplyDeltaP.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\UpdateVelocity.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\ComputeDensity.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\ApplyViscosity.comp" -Raw
$contenido += "`n`n"
$contenido += Get-Content "C:\Users\Javie\VSCode Projects\TFM\SPH-Fluid\src\graphics\compute\ResolveCollisions.comp" -Raw


# Copiar al portapapeles
$contenido | Set-Clipboard

# Confirmación
Write-Host "Contenido de ambos archivos copiado al portapapeles como texto."
