cd ..\third_party

IF EXIST .\SuperLU_3.1\SRC GOTO END

copy superlu_3.1.tar.gz superlu_3.1_copy.tar.gz
bin\gzip.exe -d superlu_3.1_copy.tar.gz
bin\tar -xvf superlu_3.1_copy.tar
del superlu_3.1_copy.tar

:END