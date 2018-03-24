
Workplaceps.dll: dlldata.obj Workplace_p.obj Workplace_i.obj
	link /dll /out:Workplaceps.dll /def:Workplaceps.def /entry:DllMain dlldata.obj Workplace_p.obj Workplace_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del Workplaceps.dll
	@del Workplaceps.lib
	@del Workplaceps.exp
	@del dlldata.obj
	@del Workplace_p.obj
	@del Workplace_i.obj
