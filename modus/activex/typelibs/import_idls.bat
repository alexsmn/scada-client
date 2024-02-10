set DIR=d:\Program Files (x86)\Modus 6.30\bin

rem tlibimp is a part of Delphi distributive.
rem tlibimp -P- -Ic+ -XM+ "%DIR%\htsde2.ocx"

rem "d:\Utils\OleWoo\oledump.exe" "%DIR%\htsde2.ocx" > htsde2.idl
rem "d:\Utils\OleWoo\oledump.exe" "%DIR%\htsde2.ocx" > htsde2.idl

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"

tlbimp "%DIR%\htsde2.ocx" /out:htsde2.dll
rem ildasm htsde2.dll /out=htsde2.idl
rem regasm htsde2.dll /tlb:htsde2.tlb /codebase
tlbexp htsde2.dll /out:htsde2.idl
