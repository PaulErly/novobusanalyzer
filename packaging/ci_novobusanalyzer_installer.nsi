!include "MUI2.nsh"

Name "NovoBusAnalyzer"
OutFile "${OUTFILE}"
InstallDir "$PROGRAMFILES\NovoBusAnalyzer"
RequestExecutionLevel admin

!define MUI_ABORTWARNING
!define MUI_ICON "..\Sources\BUSMASTER\Application\Res\novobusanalyzer.ico"
!define MUI_UNICON "..\Sources\BUSMASTER\Application\Res\Uninstaller.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE English

Section "NovoBusAnalyzer" SEC_MAIN
  SetOutPath "$INSTDIR"
  File /r "${RELEASE_DIR}\*"

  SetOutPath "$INSTDIR\Samples\CAN"
  File /r "${SAMPLE_CAN_DIR}\*.dbc"

  WriteUninstaller "$INSTDIR\uninst.exe"
SectionEnd

Section "Uninstall"
  Delete "$INSTDIR\uninst.exe"
  RMDir /r "$INSTDIR"
SectionEnd
