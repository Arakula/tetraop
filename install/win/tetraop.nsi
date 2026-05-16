;--------------------------------
; TetraOP VST3 Installer (NSIS)
;--------------------------------

!include "MUI2.nsh"
!include "FileFunc.nsh"
!insertmacro GetParameters

!ifndef VERSION
  !define VERSION "0.0.0-dev"
!endif

;--------------------------------
; General settings
;--------------------------------
Name "TetraOP ${VERSION}"
OutFile "tetraop-win64-${VERSION}-setup.exe"
InstallDir "$PROGRAMFILES64\Common Files\VST3\TetraOP"
InstallDirRegKey HKLM "Software\Tilr\TetraOP" "Install_Dir"
RequestExecutionLevel admin
ShowInstDetails show
BrandingText " "

; Custom icon
!define MUI_WELCOMEFINISHPAGE_BITMAP "assets\wizard.bmp"

;--------------------------------
; Pages
;--------------------------------
!insertmacro MUI_PAGE_WELCOME
;!insertmacro MUI_PAGE_LICENSE "..\include\EULA.txt"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_LANGUAGE "English"

;--------------------------------
; Uninstaller
;--------------------------------
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Components
;--------------------------------

Section "VST3" SecVST3
    SetOutPath "$INSTDIR"
    File /r "..\..\build\TetraOP_artefacts\Release\VST3\TetraOP.vst3"
SectionEnd

Section "Presets" SecPresets
    RMDir /r "$APPDATA\TetraOP\presets\Factory"
    SetOutPath "$APPDATA\TetraOP\presets\Factory"
    File /r "..\include\presets\Factory\*.*"
SectionEnd

Section "Wavetables" SecWavetables
    SetOutPath "$APPDATA\TetraOP\wavetables"
    File /r "..\include\wavetables\*.*"
SectionEnd

Section "Uninstaller & Registry"
    SetOutPath "$INSTDIR"
    WriteUninstaller "$INSTDIR\Uninstall.exe"
    SetRegView 64

    ; Registry entries for Add/Remove Programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TetraOP" "DisplayName" "TetraOP"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TetraOP" "UninstallString" "$INSTDIR\Uninstall.exe"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TetraOP" "Publisher" "Tilr"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TetraOP" "DisplayVersion" "${VERSION}"
SectionEnd

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
    !insertmacro MUI_DESCRIPTION_TEXT ${SecVST3} "TetraOP VST3 plugin (64-bit)."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecPresets} "Factory presets library."
    !insertmacro MUI_DESCRIPTION_TEXT ${SecWavetables} "Factory wavetables"
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
; Uninstaller Section
;--------------------------------
Section "Uninstall"
    Delete "$INSTDIR\TetraOP.vst3"
    RMDir /r "$APPDATA\TetraOP"
    Delete "$INSTDIR\Uninstall.exe"
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\TetraOP"
SectionEnd
