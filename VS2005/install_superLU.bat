cd ..\third_party

copy superlu_3.1.tar.gz superlu_3.1_copy.tar.gz
bin\gzip.exe -d superlu_3.1_copy.tar.gz
bin\tar -xvf superlu_3.1_copy.tar
rm superlu_3.1_copy.tar
