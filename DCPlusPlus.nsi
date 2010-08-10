!include "Sections.nsh"

Function GetDCPlusPlusVersion
        Exch $0
	GetDllVersion "$INSTDIR\$0" $R0 $R1
	IntOp $R2 $R0 / 0x00010000
	IntOp $R3 $R0 & 0x0000FFFF
	IntOp $R4 $R1 / 0x00010000
	IntOp $R5 $R1 & 0x0000FFFF
        StrCpy $1 "$R2.$R3$R4$R5"
        Exch $1
FunctionEnd

SetCompressor "lzma"

; The name of the installer
Name "DC++"

ShowInstDetails show
ShowUninstDetails show

Page license
Page components
Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

; The file to write
OutFile "DCPlusPlus-xxx.exe"

; The default installation directory
InstallDir $PROGRAMFILES\DC++
; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM SOFTWARE\DC++ "Install_Dir"

LicenseText "DC++ is licensed under the GPL, here's the full text!"
LicenseData "License.txt"
LicenseForceSelection checkbox

; The text to prompt the user to enter a directory
ComponentText "Welcome to the DC++ installer."
; The text to prompt the user to enter a directory
DirText "Choose a directory to install in to:"

; The stuff to install
Section "DC++ (required)" dcpp
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR

  ; Check for existing settings in the profile first
  IfFileExists "$APPDATA\DC++\*.xml" 0 check_programdir
  MessageBox MB_YESNO|MB_ICONQUESTION "A previous installation of DC++ has been found, backup settings and queue? (You can find them in $APPDATA\DC++\BACKUP later)" IDNO no_backup
  CreateDirectory "$APPDATA\DC++\BACKUP\"
  CopyFiles "$APPDATA\DC++\*.xml" "$APPDATA\DC++\BACKUP\"
  goto no_backup

check_programdir:
  ; Maybe we're upgrading from an older version so lets check the program directory
  ; Delete a possibly existing dcppboot.xml to be able to check for existing local settings 
  Delete "$INSTDIR\dcppboot.xml"
  IfFileExists "$INSTDIR\*.xml" 0 no_backup
  MessageBox MB_YESNO|MB_ICONQUESTION "A previous installation of DC++ has been found, backup settings and queue? (You can find them in $INSTDIR\BACKUP later)" IDNO no_backup
  CreateDirectory "$INSTDIR\BACKUP\"
  CopyFiles "$INSTDIR\*.xml" "$INSTDIR\BACKUP\"

no_backup:
  ; Put file there
  File "changelog.txt"
  File "cshelp.rtf"
  File "dcppboot.xml"
  File "DCPlusPlus.chm"
  File "DCPlusPlus.exe"
  File "License.txt"
  File "LICENSE-GeoIP.txt"
  File "LICENSE-OpenSSL.txt"
  File "mingwm10.dll"

  ; Add the whole locale directory
  File /r "locale" 
  
  ; Remove opencow just in case we're upgrading
  Delete "$INSTDIR\opencow.dll"
  
  ; Get DCPlusplus version we just installed and store in $1
  Push "DCPlusPlus.exe"
  Call "GetDCPlusPlusVersion"
  Pop $1

  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\DC++ "Install_Dir" "$INSTDIR"
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "InstallLocation" "$INSTDIR"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "DisplayIcon" '"$INSTDIR\DCPlusPlus.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "DisplayName" "DC++ $1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "DisplayVersion" "$1"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "Publisher" "Jacek Sieka"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "URLInfoAbout" "http://dcplusplus.sourceforge.net/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "URLUpdateInfo" "http://dcplusplus.sourceforge.net/download/"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "HelpLink" "http://dcplusplus.sourceforge.net/webhelp/"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "NoModify" "1"
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++" "NoRepair" "1"
  
  WriteUninstaller "uninstall.exe"
SectionEnd

Section "IP -> Country mappings"
  SetOutPath $INSTDIR
  File "GeoIPCountryWhois.csv"
SectionEnd

; optional section
Section "Start Menu Shortcuts"
  CreateDirectory "$SMPROGRAMS\DC++"
  CreateShortCut "$SMPROGRAMS\DC++\DC++.lnk" "$INSTDIR\DCPlusPlus.exe" "" "$INSTDIR\DCPlusPlus.exe" 0 "" "" "DC++ File Sharing Application"
  CreateShortCut "$SMPROGRAMS\DC++\License.lnk" "$INSTDIR\License.txt"
  CreateShortCut "$SMPROGRAMS\DC++\Help.lnk" "$INSTDIR\DCPlusPlus.chm"
  CreateShortCut "$SMPROGRAMS\DC++\Change Log.lnk" "$INSTDIR\ChangeLog.txt"
  CreateShortCut "$SMPROGRAMS\DC++\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
SectionEnd

Section "Store settings in your user profile folder" loc
  ; Change to nonlocal dcppboot if the checkbox left checked
  SetOutPath $INSTDIR
  File /oname=dcppboot.xml "dcppboot.nonlocal.xml" 
SectionEnd

Function .onSelChange
  ; Do not show the warning on XP or older
  StrCmp $R8 "0" skip
  ; Show the warning only once
  StrCmp $R9 "1" skip
  SectionGetFlags ${loc} $0
  IntOp $0 $0 & ${SF_SELECTED}
  StrCmp $0 ${SF_SELECTED} skip
    StrCpy $R9 "1"
    MessageBox MB_OK|MB_ICONEXCLAMATION "If you want to keep your settings in the program directory using Windows Vista or later make sure that you DO NOT install DC++ to the 'Program files' folder!!! This can lead to abnormal behaviour like loss of settings or downloads!"
skip:
FunctionEnd


Function .onInit
  ; Check for Vista+
  ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
  IfErrors xp_or_below nt
nt:
  StrCmp $R8 '6.0' vistaplus
  StrCmp $R8 '6.1' 0 xp_or_below
vistaplus:
  StrCpy $R8 "1"
  goto end
xp_or_below:
  StrCpy $R8 "0"
end:
  ; Set the program component really required (read only)
  IntOp $0 ${SF_SELECTED} | ${SF_RO}
  SectionSetFlags ${dcpp} $0
FunctionEnd

; uninstall stuff

UninstallText "This will uninstall DC++ by removing installed program files and locale data. WARNING: the contents of the locale data folder ($INSTDIR\locale) will be completely erased!"

; special uninstall section.
Section "un.Uninstall"
  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\DC++"
  DeleteRegKey HKLM SOFTWARE\DC++
  ; remove files
  Delete "$INSTDIR\DCPlusPlus.exe"
  Delete "$INSTDIR\DCPlusPlus.chm"
  Delete "$INSTDIR\dcppboot.xml"
  Delete "$INSTDIR\License-GeoIP.txt"
  Delete "$INSTDIR\License.txt"
  Delete "$INSTDIR\ChangeLog.txt"
  Delete "$INSTDIR\cshelp.rtf"
  Delete "$INSTDIR\LICENSE-OpenSSL.txt"
  Delete "$INSTDIR\GeoIPCountryWhois.csv"
  Delete "$INSTDIR\mingwm10.dll"

  ; Delete the whole locale directory
  RMDir /r "$INSTDIR\locale"

  ; Remove registry entries
  ; Assume they are all registered to us
  DeleteRegKey HKCU "Software\Classes\dchub"
  DeleteRegKey HKCU "Software\Classes\adc"
  DeleteRegKey HKCU "Software\Classes\adcs"
  DeleteRegKey HKCU "Software\Classes\magnet"
  ; MUST REMOVE UNINSTALLER, too
  Delete $INSTDIR\uninstall.exe
  ; remove shortcuts, if any.
  Delete "$SMPROGRAMS\DC++\*.*"
  ; remove directories used.
  RMDir "$SMPROGRAMS\DC++"

  MessageBox MB_YESNO|MB_ICONQUESTION "Also remove queue, settings and the complete DC++ program folder ($INSTDIR) with all of its subfolders?" IDYES kill_dir

  RMDir "$INSTDIR"
  goto end_uninstall

kill_dir:
  ; delete program directory
  RMDir /r "$INSTDIR"
  ; delete profile data directories
  RMDir /r "$APPDATA\DC++"
  RMDir /r "$LOCALAPPDATA\DC++"  

end_uninstall:

SectionEnd

; eof
