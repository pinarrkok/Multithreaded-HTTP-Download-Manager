SRC=http_clientV2.c
EXE=hw1_20160808013.exe
FLAGS=-pthread -lm

install : $(SRC)
	gcc -o $(EXE) $(SRC) $(FLAGS)
