FOR %%i IN (..\www\*.html) DO 7z\7z.exe a -tgzip -mx9 -mmt4 "..\data\www\%%i.gz" "%%i"
FOR %%i IN (..\www\*.css) DO 7z\7z.exe a -tgzip -mx9 -mmt4 "..\data\www\%%i.gz" "%%i"