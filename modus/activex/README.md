# OleView

Use OleView to import libraries.

Open in `d:\Program Files (x86)\Modus 6.30\bin`:
- `htsde2.ocx`
- `sdecore.tlb`

Importing from IDLs is not working.
- The generated IDL depends on TLBs.


```batch
set DIR=d:\Program Files (x86)\Modus 6.30\bin
```

# Delphi `tlibimp`

```batch
tlibimp -P- -Ic+ -XM+ "%DIR%\htsde2.ocx"
```

# OleWoo

```batch
"d:\Utils\OleWoo\oledump.exe" "%DIR%\htsde2.ocx" > htsde2.idl
"d:\Utils\OleWoo\oledump.exe" "%DIR%\htsde2.ocx" > htsde2.idl
```

# .NET `tlbimp`

https://stackoverflow.com/questions/4990045/how-to-generate-type-library-from-unmanaged-com-dll

```
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
tlbimp "%DIR%\htsde2.ocx" /out:htsde2.dll
ildasm htsde2.dll /out=htsde2.idl
```

# SysInternals

```batch
"d:\Utils\ResourcesExtract.exe" "%DIR%\htsde2.ocx"
```
