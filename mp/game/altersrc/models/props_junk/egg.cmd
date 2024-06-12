@echo off
setlocal enabledelayedexpansion
set "prefix=\Users\Moon\Documents\GitHub\alter-source\mp\game\altersrc\"
for /r %%i in (*.mdl) do (
    set "path=%%~pi%%~nxi"
    echo !path:%prefix%=!
)
pause