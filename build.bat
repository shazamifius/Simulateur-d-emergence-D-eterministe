@ECHO OFF
ECHO ==================================================
ECHO  Script de Compilation pour le Simulateur SED
ECHO ==================================================
ECHO.

REM --- Etape 1: Nettoyage de l'ancien dossier de build ---
ECHO Nettoyage de l'ancien dossier de compilation...
IF EXIST build (
    RMDIR /S /Q build
)
ECHO Nettoyage termine.
ECHO.

REM --- Etape 2: Creation du dossier de build ---
ECHO Creation d'un nouveau dossier de compilation...
MKDIR build
IF NOT EXIST build (
    ECHO ERREUR: Impossible de creer le dossier 'build'.
    PAUSE
    EXIT /B 1
)
CD build
ECHO Dossier 'build' cree.
ECHO.

REM --- Etape 3: Configuration avec CMake ---
ECHO Configuration du projet avec CMake...
cmake ..
IF %ERRORLEVEL% NEQ 0 (
    ECHO ERREUR: La configuration CMake a echoue.
    PAUSE
    EXIT /B 1
)
ECHO Configuration terminee.
ECHO.

REM --- Etape 4: Compilation avec Make ---
ECHO Compilation du projet...
make
IF %ERRORLEVEL% NEQ 0 (
    ECHO ERREUR: La compilation a echoue.
    PAUSE
    EXIT /B 1
)
ECHO Compilation terminee.
ECHO.

REM --- Etape 5: Lancement de l'application ---
ECHO Lancement de sed_lab.exe...
IF EXIST sed_lab.exe (
    start sed_lab.exe
) ELSE (
    ECHO ERREUR: L'executable sed_lab.exe n'a pas ete trouve.
)
ECHO.

ECHO Script termine.
PAUSE
