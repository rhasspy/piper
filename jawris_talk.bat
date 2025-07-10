@echo off
setlocal enabledelayedexpansion

:: === Absoluter Projektpfad ===
set "JAWRIS_PATH=C:\Jawris_Beta\jawris-piper"
cd /d "%JAWRIS_PATH%"

:: === Richtiger Python-Interpreter
set "PYTHON_EXEC=C:\Jawris_Beta\jawris-env\Scripts\python.exe"
set "PYTHON_SCRIPT=jawris_speak.py"

echo -----------------------------------
echo 🧬 Jawris Sprachmodul gestartet
echo -----------------------------------
set /p TEXT="Was soll Jawris sagen? "
echo.

:: Stil-Auswahl
echo Verfügbare Stile: neutral / calm / reflective / alert / mythic
set /p STYLE="Gewuenschter Stil: "
echo.

:: Debug-Ausgabe
echo 🧪 TEXT = %TEXT%
echo 🧪 STYLE = %STYLE%
echo 🧪 Script = %JAWRIS_PATH%\%PYTHON_SCRIPT%
echo 🧪 Python = %PYTHON_EXEC%
echo.

:: Starte Python
"%PYTHON_EXEC%" "%PYTHON_SCRIPT%" "%TEXT%" "%STYLE%"
if %ERRORLEVEL% neq 0 (
    echo 🔥 Ein Fehler ist aufgetreten beim Ausführen von Python.
) else (
    echo ✅ Ausführung abgeschlossen.
)
pause
