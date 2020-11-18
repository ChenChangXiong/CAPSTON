
@echo off & title 文件监控
color 0A & mode 36,6
 
::设置要监控的文件夹  
set MtrDir=Y:\task

::设置复制文件源目录   

set SourceDir=C:\ftp_files

::设置复制到路径    \\APLEX-WS\WS-Share

set NetDir=Y:
set tmp=C:


SET GenFile1=*.txt


echo 正在初始化记录文件 ...
(for /f "delims=" %%a in ('dir /a-d/s/b "%MtrDir%"') do (
    echo "%%~a"
))>"%tmp%\oFiles.Lst"

:: 只会查看%MtrDir%\52*文件 而不是%MtrDir% 查看是否存在 在删除
:Loop
cls & set "Change="
echo 正在监控文件夹是否更新 ...
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