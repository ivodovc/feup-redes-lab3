echo Starting compile
del trans.exe recv.exe link_library.o
gcc link_library.c -c -o"link_library.o"
gcc trans.c -otrans link_library.o
gcc recv.c -orecv link_library.o
echo Done