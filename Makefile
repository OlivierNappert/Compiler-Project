OBJ=tp.o tp_l.o tp_y.o verif.o print.o code.o
CC=gcc
CFLAGS=-Wall -ansi -I./ -g 
LDFLAGS= -g -lfl
tp : $(OBJ)
	$(CC) -o tp $(OBJ) $(LDFLAGS)

test_lex : tp_l.o test_lex.c tp_y.h tp.h
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-implicit-function-declaration -o test_lex test_lex.c  tp_l.o $(LDFLAGS)
	
tp.c :
	echo ''

tp.o: tp.c tp_y.h tp.h
	$(CC) $(CFLAGS) -c tp.c

verif.o: verif.c tp_y.h tp.h
	$(CC) $(CFLAGS) -c verif.c

code.o: code.c tp_y.h tp.h
	$(CC) $(CFLAGS) -c code.c

print.o: print.c tp_y.h tp.h
	$(CC) $(CFLAGS) -c print.c

tp_l.o: tp_l.c tp_y.h
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-implicit-function-declaration -c tp_l.c

tp_l.c : tp.l tp_y.h tp.h
	flex --yylineno -otp_l.c tp.l

tp_y.o : tp_y.c
	$(CC) $(CFLAGS) -c tp_y.c

tp_y.h tp_y.c : tp.y tp.h
	bison -v -b tp_y -o tp_y.c -d tp.y

.Phony: clean

clean:
	rm -f *~ tp.exe* ./tp *.o tp_l.* tp_y.* test_lex
	rm -f test/*~ test/*.out test/*/*~ test/*/*.out
