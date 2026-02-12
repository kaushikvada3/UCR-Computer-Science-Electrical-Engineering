!ifndef APP_VERSION
  !define APP_VERSION "0.0.0"
!endif
!ifndef APP_DIST_DIR
  !error "APP_DIST_DIR must be defined"
!endif
!ifndef OUT_FILE
  !error "OUT_FILE must be defined"
!endif
!ifndef ICON_FILE
  !define ICON_FILE ""
!endif

Name "BMS Dashboard ${APP_VERSION}"
OutFile "${OUT_FILE}"
InstallDir "$PROGRAMFILES\\BMS Dashboard"
RequestExecutionLevel admin
ShowInstDetails show
ShowUnInstDetails show

!if "${ICON_FILE}" != ""
Icon "${ICON_FILE}"
UninstallIcon "${ICON_FILE}"
!endif

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "Install"
  SetOutPath "$INSTDIR"
  File /r "${APP_DIST_DIR}\\*"

  CreateDirectory "$SMPROGRAMS\\BMS Dashboard"
  CreateShortcut "$SMPROGRAMS\\BMS Dashboard\\BMS Dashboard.lnk" "$INSTDIR\\BMSDashboard.exe"
  CreateShortcut "$DESKTOP\\BMS Dashboard.lnk" "$INSTDIR\\BMSDashboard.exe"

  WriteUninstaller "$INSTDIR\\Uninstall.exe"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "DisplayName" "BMS Dashboard"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "Publisher" "UCR"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "UninstallString" '"$INSTDIR\\Uninstall.exe"'
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "NoModify" 1
  WriteRegDWORD HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "NoRepair" 1
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\\BMS Dashboard.lnk"
  Delete "$SMPROGRAMS\\BMS Dashboard\\BMS Dashboard.lnk"
  RMDir "$SMPROGRAMS\\BMS Dashboard"

  RMDir /r "$INSTDIR"
  DeleteRegKey HKLM "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard"
SectionEnd
