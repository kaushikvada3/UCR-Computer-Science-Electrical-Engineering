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

!include "LogicLib.nsh"
!include "FileFunc.nsh"
!insertmacro GetParameters
!insertmacro GetOptions

Var AutoCloseApp
Var WaitPid
Var UpdateMode
Var UpdateRequiresReboot
Var RetryCount

Name "BMS Dashboard ${APP_VERSION}"
OutFile "${OUT_FILE}"
InstallDir "$LOCALAPPDATA\\Programs\\BMS Dashboard"
RequestExecutionLevel user
ShowInstDetails show
ShowUnInstDetails show
SetCompressor /SOLID lzma
SetCompressorDictSize 64

!if "${ICON_FILE}" != ""
Icon "${ICON_FILE}"
UninstallIcon "${ICON_FILE}"
!endif

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Function .onInit
  StrCpy $AutoCloseApp "1"
  StrCpy $WaitPid ""
  StrCpy $UpdateMode "0"

  ${GetParameters} $0
  ${GetOptions} $0 "/AUTOCLOSEAPP=" $1
  ${If} $1 != ""
    StrCpy $AutoCloseApp $1
  ${EndIf}
  ${GetOptions} $0 "/WAITPID=" $1
  ${If} $1 != ""
    StrCpy $WaitPid $1
  ${EndIf}
  ${GetOptions} $0 "/UPDATE_MODE=" $1
  ${If} $1 != ""
    StrCpy $UpdateMode $1
  ${EndIf}

  Call PreflightForRunningApp
FunctionEnd

Function PreflightForRunningApp
  ${If} $WaitPid != ""
    DetailPrint "Waiting for BMS Dashboard process PID $WaitPid to exit..."
wait_pid_loop:
    nsExec::ExecToLog "cmd /c tasklist /FI $\"PID eq $WaitPid$\" | find $\"$WaitPid$\" >NUL"
    Pop $0
    ${If} $0 == "0"
      Sleep 250
      Goto wait_pid_loop
    ${EndIf}
  ${EndIf}

  nsExec::ExecToLog "cmd /c tasklist /FI $\"IMAGENAME eq BMSDashboard.exe$\" | find /I $\"BMSDashboard.exe$\" >NUL"
  Pop $0
  ${If} $0 == "0"
    ${If} $AutoCloseApp == "1"
      DetailPrint "Detected running BMSDashboard.exe. Force-closing process..."
      nsExec::ExecToLog "cmd /c taskkill /F /IM BMSDashboard.exe >NUL 2>NUL"
      Pop $1
      Sleep 700
    ${Else}
      MessageBox MB_ICONEXCLAMATION|MB_OK "BMS Dashboard is currently running. Please close it and run setup again."
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

Function WaitForExeUnlock
  ${IfNot} ${FileExists} "$INSTDIR\\BMSDashboard.exe"
    Return
  ${EndIf}

  StrCpy $RetryCount "0"
wait_lock_loop:
  ClearErrors
  FileOpen $0 "$INSTDIR\\BMSDashboard.exe" a
  ${IfNot} ${Errors}
    FileClose $0
    Return
  ${EndIf}

  IntOp $RetryCount $RetryCount + 1
  ${If} $RetryCount >= 40
    DetailPrint "BMSDashboard.exe is still locked after 20 seconds; using reboot fallback if needed."
    Return
  ${EndIf}

  Sleep 500
  Goto wait_lock_loop
FunctionEnd

Section "Install"
  StrCpy $UpdateRequiresReboot "0"

  SetOutPath "$INSTDIR"
  Call WaitForExeUnlock

  SetOverwrite try
  ClearErrors
  File /nonfatal /r "${APP_DIST_DIR}\\*"
  ${If} ${Errors}
    DetailPrint "Non-fatal copy reported at least one locked file. Continuing with executable fallback."
    ClearErrors
  ${EndIf}

  SetOutPath "$INSTDIR"
  File /oname=BMSDashboard.exe.new "${APP_DIST_DIR}\\BMSDashboard.exe"

  ClearErrors
  Delete "$INSTDIR\\BMSDashboard.exe"
  Rename "$INSTDIR\\BMSDashboard.exe.new" "$INSTDIR\\BMSDashboard.exe"
  ${If} ${Errors}
    DetailPrint "Could not replace BMSDashboard.exe immediately. Queuing replacement for reboot."
    ClearErrors
    Delete /REBOOTOK "$INSTDIR\\BMSDashboard.exe"
    Rename /REBOOTOK "$INSTDIR\\BMSDashboard.exe.new" "$INSTDIR\\BMSDashboard.exe"
    ${If} ${Errors}
      MessageBox MB_ICONSTOP|MB_OK "Setup could not stage BMSDashboard.exe replacement. Please reboot and run setup again."
      Abort
    ${Else}
      StrCpy $UpdateRequiresReboot "1"
      SetRebootFlag true
    ${EndIf}
  ${EndIf}

  CreateDirectory "$SMPROGRAMS\\BMS Dashboard"
  CreateShortcut "$SMPROGRAMS\\BMS Dashboard\\BMS Dashboard.lnk" "$INSTDIR\\BMSDashboard.exe"
  CreateShortcut "$DESKTOP\\BMS Dashboard.lnk" "$INSTDIR\\BMSDashboard.exe"

  WriteUninstaller "$INSTDIR\\Uninstall.exe"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "DisplayName" "BMS Dashboard"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "DisplayVersion" "${APP_VERSION}"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "Publisher" "UCR"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "InstallLocation" "$INSTDIR"
  WriteRegStr HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "UninstallString" '"$INSTDIR\\Uninstall.exe"'
  WriteRegDWORD HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "NoModify" 1
  WriteRegDWORD HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard" "NoRepair" 1

  ${If} $UpdateRequiresReboot == "1"
    MessageBox MB_ICONINFORMATION|MB_OK "Update staged successfully. Restart Windows to finish replacing BMSDashboard.exe."
  ${EndIf}
SectionEnd

Section "Uninstall"
  Delete "$DESKTOP\\BMS Dashboard.lnk"
  Delete "$SMPROGRAMS\\BMS Dashboard\\BMS Dashboard.lnk"
  RMDir "$SMPROGRAMS\\BMS Dashboard"

  RMDir /r "$INSTDIR"
  DeleteRegKey HKCU "Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\BMSDashboard"
SectionEnd
