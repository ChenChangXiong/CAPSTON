
@echo off & title �ļ����
color 0A & mode 36,6
 
::����Ҫ��ص��ļ���  
set MtrDir=Y:\task

::���ø����ļ�ԴĿ¼   

set SourceDir=C:\ftp_files

::���ø��Ƶ�·��    \\APLEX-WS\WS-Share

set NetDir=Y:
set tmp=C:


SET GenFile1=*.txt


echo ���ڳ�ʼ����¼�ļ� ...
(for /f "delims=" %%a in ('dir /a-d/s/b "%MtrDir%"') do (
    echo "%%~a"
))>"%tmp%\oFiles.Lst"

:: ֻ��鿴%MtrDir%\52*�ļ� ������%MtrDir% �鿴�Ƿ���� ��ɾ��
:Loop
cls & set "Change="
echo ���ڼ���ļ����Ƿ���� ...
for /f "delims=" %%a in ('dir /a-d/s/b "%MtrDir%"') do (
     findstr /i "^\"%%~a\"$" "%tmp%\oFiles.Lst" >nul || (

          if exist "Y:\task\*01_*" (
		 set Change=1
		 goto L1  

      	  )
    )
)

:L1
if defined Change (
    setlocal enabledelayedexpansion
    XCOPY  "Y:\task\*01_*"  "C:\ftp_files\config"   /y  

    if !errorlevel! == 0 (
          echo !errorlevel!
	  endlocal  enabledelayedexpansion
          del  /f /q    "Y:\task\*01_*"
     ) else (
        echo  -----
	TIMEOUT  /T  2
    	endlocal  enabledelayedexpansion

     )

   
)

goto Loop