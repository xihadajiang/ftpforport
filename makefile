OBJ1=obj/FTPserver.o obj/tcpfunc.o
OBJ2=obj/FTPclient.o obj/tcpfunc.o

INCLUDE=-I./ 
LDFLAGS=-lc -lm
CC=cc -Wall -m64 -g
MAKEFILE=makefile

up: $(HOME)/bin/FTPserver $(HOME)/bin/FTPclient
	@echo -e "\nFTP MAKED!"

$(HOME)/bin/FTPserver:$(OBJ1) 
	@rm -rf $(HOME)/bin/FTPserver
	$(CC) -o $(HOME)/bin/FTPserver $(OBJ1) $(LDFLAGS) 

$(HOME)/bin/FTPclient:$(OBJ2) 
	@rm -rf $(HOME)/bin/FTPclient
	$(CC) -o $(HOME)/bin/FTPclient $(OBJ2) $(LDFLAGS) 


obj/FTPserver.o:FTPserver.c
	$(CC) $(INCLUDE) -c FTPserver.c
	@mv FTPserver.o obj/
obj/FTPclient.o:FTPclient.c
	$(CC) $(INCLUDE) -c FTPclient.c
	@mv FTPclient.o obj/
obj/tcpfunc.o:tcpfunc.c
	$(CC) -s $(INCLUDE) -c tcpfunc.c
	@mv tcpfunc.o obj/
clean:
	@rm -f obj/*.o
