# Build .msi sequence

[WiX toolset download link](https://github.com/wixtoolset/wix3/releases)

Put all product files to the `files/` directory.

Create file list command:

```
heat.exe dir files\ -dr INSTALLDIR -sreg -srd -sfrag -ag -cg ClientFiles -var var.HarvestAppDir -platform='x64' -t collect-transform.xsl -out collect.wxs
```

Compile command:

```
candle.exe -dPlatform=x64 -arch x64 -dHarvestAppDir="files" -ext WixUtilExtension collect.wxs shellext.wxs product.wxs
```

Create MSI command:

```
light.exe -sw1076 -ext WixUIExtension -ext WixUtilExtension collect.wixobj product.wixobj shellext.wixobj -out owncloud-client.msi
```
