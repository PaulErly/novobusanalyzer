!include "MUI2.nsh"

Name "NovoBusAnalyzer"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES\NovoBusAnalyzer"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!define MUI_ICON "..\Sources\BUSMASTER\Application\Res\novobusanalyzer.ico"
!define MUI_UNICON "..\Sources\BUSMASTER\Application\Res\Uninstaller.ico"

Page welcome
Page directory
Page instfiles
UninstPage instfiles

Section "NovoBusAnalyzer" SEC_MAIN
  SetOutPath "$INSTDIR"
  File /r "${RELEASE_DIR}\*"

  SetOutPath "$INSTDIR\Samples\CAN"
  File /oname=dbc_smoke.dbc "${SAMPLE_DBC}"

  WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\uninst.exe"
  RMDir /r "$INSTDIR"
SectionEnd
