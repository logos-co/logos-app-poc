; Windows installer for Logos Basecamp.
;
; ONE binary, TWO modes, chosen on the first page:
;   install   -- per-user tree under %LOCALAPPDATA%\Programs, Start Menu entry,
;                uninstaller registered in Add/Remove Programs
;   portable  -- the same files extracted to a folder of the user's choosing,
;                with no shortcut, no registry key and no uninstaller
;
; The two modes ship IDENTICAL bits. There is no portable/dev split to make
; here: on Windows that distinction is compile-time (LOGOS_PORTABLE_BUILD), and
; the bundle this wraps is already the portable variant.
;
; Per-user, never Program Files, and that is load-bearing rather than a
; courtesy: Basecamp installs modules at runtime through lgpm, into its own
; tree. An admin-owned Program Files install would make every such install fail
; for the user who is running it.

Unicode true
ManifestDPIAware true

; 9000 fires because the output is named ...-setup.exe, which puts Windows'
; installer-detection heuristic in play. Suppressed rather than renamed: that
; heuristic only AUTO-ELEVATES an image carrying no requestedExecutionLevel,
; and RequestExecutionLevel below writes one. What is left is the AppCompat
; shim load, which every conventionally-named installer takes.
!pragma warning disable 9000
SetCompressor /SOLID lzma
SetCompressorDictSize 64

!include "MUI2.nsh"
!include "nsDialogs.nsh"
!include "LogicLib.nsh"
!include "FileFunc.nsh"

!define REGROOT   "Software\${APPNAME}"
!define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APPNAME}"

Name    "${APPNAME} ${APPVERSION}"
OutFile "${OUTFILE}"
BrandingText "${APPNAME} ${APPVERSION}"
RequestExecutionLevel user
InstallDir "$LOCALAPPDATA\Programs\${APPNAME}"
ShowInstDetails show
ShowUninstDetails show

VIProductVersion "${APPVERSION_QUAD}"
VIAddVersionKey "ProductName"     "${APPNAME}"
VIAddVersionKey "ProductVersion"  "${APPVERSION}"
VIAddVersionKey "FileVersion"     "${APPVERSION}"
VIAddVersionKey "CompanyName"     "${APPPUBLISHER}"
VIAddVersionKey "LegalCopyright"  "${APPPUBLISHER}"
VIAddVersionKey "FileDescription" "${APPNAME} installer"

Var Portable          ; 1 = extract only, 0 = install
Var WantDesktopIcon
Var RadioInstall
Var RadioPortable
Var CheckDesktop

!define MUI_ICON   "${APPICON}"
!define MUI_UNICON "${APPICON}"
!define MUI_ABORTWARNING

!insertmacro MUI_PAGE_WELCOME
Page custom ModePageCreate ModePageLeave
!define MUI_PAGE_CUSTOMFUNCTION_PRE DirectoryPagePre
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!define MUI_FINISHPAGE_RUN "$INSTDIR\bin\${APPEXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Run ${APPNAME}"
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

Function .onInit
    StrCpy $Portable 0
    StrCpy $WantDesktopIcon 1
FunctionEnd

; Windows keeps a running image open for read and delete but NOT for write, so
; an append open is a plugin-free in-use test. Worth the lines: RMDir /r deletes
; what it can and still reports success, so uninstalling over a running app
; leaves a half-removed tree behind an exit code of 0. Measured -- 1825 files
; survived, status 0. Installing over one produces a mixed tree the same way.
!macro RefuseIfRunning UN
Function ${UN}RefuseIfRunning
    ${IfNot} ${FileExists} "$INSTDIR\bin\${APPEXE}"
        Return
    ${EndIf}
    ClearErrors
    FileOpen $0 "$INSTDIR\bin\${APPEXE}" a
    ${If} ${Errors}
        ${IfNot} ${Silent}
            MessageBox MB_ICONSTOP \
                "${APPNAME} is still running. Close it and run this again."
        ${EndIf}
        DetailPrint "${APPNAME} is running; nothing was changed."
        SetErrorLevel 1
        Abort
    ${EndIf}
    FileClose $0
FunctionEnd
!macroend
!insertmacro RefuseIfRunning ""
!insertmacro RefuseIfRunning "un."

; ---------------------------------------------------------------------------
; Mode page
; ---------------------------------------------------------------------------
Function ModePageCreate
    !insertmacro MUI_HEADER_TEXT "Installation type" \
        "Choose how ${APPNAME} should be put on this machine."
    nsDialogs::Create 1018
    Pop $0
    ${If} $0 == error
        Abort
    ${EndIf}

    ${NSD_CreateRadioButton} 0 0 100% 12u "&Install (recommended)"
    Pop $RadioInstall
    ${NSD_CreateLabel} 12u 14u 94% 26u \
        "Installs for the current user only -- no administrator prompt. Adds a \
Start Menu entry and an uninstaller listed in Add or remove programs."

    ${NSD_CreateRadioButton} 0 46u 100% 12u "&Portable (extract only)"
    Pop $RadioPortable
    ${NSD_CreateLabel} 12u 60u 94% 26u \
        "Unpacks the same files to a folder you choose and stops there: no \
shortcut, no registry entry, no uninstaller. Delete the folder to remove it."

    ${NSD_CreateCheckBox} 0 96u 100% 12u "Create a &desktop shortcut"
    Pop $CheckDesktop

    ${If} $Portable == 1
        ${NSD_SetState} $RadioPortable ${BST_CHECKED}
    ${Else}
        ${NSD_SetState} $RadioInstall ${BST_CHECKED}
    ${EndIf}
    ${NSD_SetState} $CheckDesktop $WantDesktopIcon
    ${NSD_OnClick} $RadioInstall  ModeChanged
    ${NSD_OnClick} $RadioPortable ModeChanged
    Call ModeChanged

    nsDialogs::Show
FunctionEnd

; The desktop-shortcut box is meaningless in portable mode -- that mode's whole
; contract is that it writes nothing outside the folder.
Function ModeChanged
    ${NSD_GetState} $RadioPortable $0
    ${If} $0 == ${BST_CHECKED}
        EnableWindow $CheckDesktop 0
    ${Else}
        EnableWindow $CheckDesktop 1
    ${EndIf}
FunctionEnd

Function ModePageLeave
    ${NSD_GetState} $RadioPortable $Portable
    ${NSD_GetState} $CheckDesktop  $WantDesktopIcon
    ${If} $Portable == ${BST_CHECKED}
        StrCpy $Portable 1
        StrCpy $INSTDIR "$DESKTOP\${APPNAME}"
    ${Else}
        StrCpy $Portable 0
        StrCpy $INSTDIR "$LOCALAPPDATA\Programs\${APPNAME}"
    ${EndIf}
FunctionEnd

Function DirectoryPagePre
    ${If} $Portable == 1
        !insertmacro MUI_HEADER_TEXT "Choose a folder" \
            "${APPNAME} will be extracted here and will write nothing else."
    ${EndIf}
FunctionEnd

; ---------------------------------------------------------------------------
Section "${APPNAME}" SecMain
    Call RefuseIfRunning
    SetOutPath "$INSTDIR"
    File /r "${STAGEDIR}/*"
    File "/oname=${APPNAME}.ico" "${APPICON}"

    ${If} $Portable == 1
        DetailPrint "Portable extraction -- no shortcuts, registry keys or uninstaller written."
        Return
    ${EndIf}

    CreateShortcut "$SMPROGRAMS\${APPNAME}.lnk" "$INSTDIR\bin\${APPEXE}" "" \
                   "$INSTDIR\${APPNAME}.ico"
    ${If} $WantDesktopIcon == ${BST_CHECKED}
        CreateShortcut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\bin\${APPEXE}" "" \
                       "$INSTDIR\${APPNAME}.ico"
    ${EndIf}

    WriteUninstaller "$INSTDIR\Uninstall.exe"
    WriteRegStr HKCU "${REGROOT}" "InstallDir" "$INSTDIR"

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    WriteRegStr   HKCU "${UNINSTKEY}" "DisplayName"     "${APPNAME}"
    WriteRegStr   HKCU "${UNINSTKEY}" "DisplayVersion"  "${APPVERSION}"
    WriteRegStr   HKCU "${UNINSTKEY}" "Publisher"       "${APPPUBLISHER}"
    WriteRegStr   HKCU "${UNINSTKEY}" "DisplayIcon"     "$INSTDIR\${APPNAME}.ico"
    WriteRegStr   HKCU "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr   HKCU "${UNINSTKEY}" "UninstallString" "$\"$INSTDIR\Uninstall.exe$\""
    WriteRegStr   HKCU "${UNINSTKEY}" "QuietUninstallString" "$\"$INSTDIR\Uninstall.exe$\" /S"
    WriteRegDWORD HKCU "${UNINSTKEY}" "EstimatedSize"   "$0"
    WriteRegDWORD HKCU "${UNINSTKEY}" "NoModify"        1
    WriteRegDWORD HKCU "${UNINSTKEY}" "NoRepair"        1
SectionEnd

Section "Uninstall"
    ; Refuse to recurse through a directory that is not ours. $INSTDIR comes
    ; from the registry, and RMDir /r on a wrong one is unrecoverable.
    ${IfNot} ${FileExists} "$INSTDIR\bin\${APPEXE}"
        ; Never a dialog under /S. NSIS does not suppress MessageBox in silent
        ; mode, and Add/Remove Programs can invoke a quiet uninstall with no
        ; desktop to draw on -- the box would then block forever, invisibly.
        ${IfNot} ${Silent}
            MessageBox MB_ICONSTOP \
                "$INSTDIR does not look like a ${APPNAME} installation; nothing was removed."
        ${EndIf}
        DetailPrint "$INSTDIR is not a ${APPNAME} installation; nothing removed."
        Abort
    ${EndIf}
    Call un.RefuseIfRunning

    Delete "$SMPROGRAMS\${APPNAME}.lnk"
    Delete "$DESKTOP\${APPNAME}.lnk"
    RMDir /r "$INSTDIR"
    DeleteRegKey HKCU "${UNINSTKEY}"
    DeleteRegKey HKCU "${REGROOT}"

    ; %APPDATA%\${APPNAME} is deliberately left alone: it holds the user's
    ; logs, installed modules and module data, and an uninstall is not a
    ; request to lose them.
SectionEnd
