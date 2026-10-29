@echo off
set PATH=C:\BIN
set INCLUDE=C:\INCLUDE
set LIB=C:\LIB
if exist BUILD.OK del BUILD.OK
CL /nologo /c /G0 /AS /Os /W3 PAINT.C QA.C MEMQA.C CAPQA.C > BUILD.LOG
if errorlevel 1 goto failed
LINK /NOI /MAP /CP:4096 PAINT.OBJ,PAINT.EXE,PAINT.MAP,,; >> BUILD.LOG
if errorlevel 1 goto failed
LINK /NOI /MAP /CP:4096 QA.OBJ,QA.EXE,QA.MAP,,; >> BUILD.LOG
if errorlevel 1 goto failed
LINK /NOI /MAP /CP:4096 MEMQA.OBJ,MEMQA.EXE,MEMQA.MAP,,; >> BUILD.LOG
if errorlevel 1 goto failed
LINK /NOI /MAP /CP:4096 CAPQA.OBJ,CAPQA.EXE,CAPQA.MAP,,; >> BUILD.LOG
if errorlevel 1 goto failed
echo PASS>BUILD.OK
goto end
:failed
echo FAIL>>BUILD.LOG
:end
