:: %1 MediaList.txt file
:: %2 destination $(OutDir)
:: %3 source1 Common Samples Media Dir
:: %4 source2 Sample Media Dir
:: %5 optional separate destination for common samples media dir
:: %6 optional separate source common built dir
:: Note that shader build steps are deployed automatically because they write output into $(OutDir)

@echo off

if not exist %~1 (
::  echo MediaList %~1 isn't found, skipping Copying Media step
    goto :EOF
)

SETLOCAL ENABLEDELAYEDEXPANSION

SET COMMON_MEDIA_DIR=%~3
SET SAMPLE_MEDIA_DIR=%~4

echo Copying Media listed in %1
echo Common media dir = %COMMON_MEDIA_DIR%
echo Sample media dir = %SAMPLE_MEDIA_DIR%

if "%~6" == "" (
   SET COMMON_MEDIA_BUILT_DIR=%COMMON_MEDIA_DIR%
) else (
   SET COMMON_MEDIA_BUILT_DIR=%~6
   echo Common built media dir = !COMMON_MEDIA_BUILT_DIR!
)


if "%~5" == "" (
   SET OUTDIR_COMMON=%~2
   SET OUTDIR_SAMPLE=%~2

   echo Destination = !OUTDIR_COMMON!
) else (
   SET OUTDIR_COMMON=%~2
   SET OUTDIR_SAMPLE=%~5

   echo Destination common = !OUTDIR_COMMON!
   echo Destination sample = !OUTDIR_SAMPLE!
)

for /f "usebackq tokens=1-3" %%i in (%~1) do (
    robocopy  "%%i" "%%j" "%%k" /s /DCOPY:T /a-:RA /MT /sl /xo /v /r:5 /ts /np /njh /njs /ndl
    if %errorlevel% GTR 3 goto fail
)

:fail

SET COMMON_MEDIA_BUILT_DIR=
SET OUTDIR_COMMON=
SET OUTDIR_SAMPLE=
SET COMMON_MEDIA_DIR=
SET SAMPLE_MEDIA_DIR=

ENDLOCAL

if %errorlevel% LEQ 3 exit /B 0