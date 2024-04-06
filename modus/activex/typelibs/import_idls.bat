set DIR=c:\Program Files (x86)\Modus520\bin

rem tlibimp is a part of Delphi distributive.
rem tlibimp -P- -Ic+ -XM+ "%DIR%\htsde2.ocx"

rem "d:\Utils\OleWoo\oledump.exe" "%DIR%\htsde2.ocx" > htsde2.idl
rem "d:\Utils\OleWoo\oledump.exe" "%DIR%\htsde2.ocx" > htsde2.idl

rem call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

rem tlbimp "%DIR%\htsde2.ocx" /out:htsde2.dll
rem ildasm htsde2.dll /out=htsde2.idl
rem regasm htsde2.dll /tlb:htsde2.tlb /codebase
rem tlbexp htsde2.dll /out:htsde2.idl


copy /b /y "%DIR%\sdecore.tlb" .
copy /b /y "%DIR%\sde_electric.tlb" .
"d:\Utils\ResourcesExtract.exe" "%DIR%\htsde2.ocx"
