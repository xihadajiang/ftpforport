#directory options
LIBDIR=../lib
INCDIR=../incl
OBJDIR=../obj

# compile options
CCCC=cc $(INCLUDE) $(LIBS) -b elf -s -o
CC=cc $(INCLUDE) $(LIBS) -O -c -o
OBJS=   $(OBJDIR)/updatemac.o \
        $(OBJDIR)/md5_dgst.o \
		$(OBJDIR)/md5.o
LIBS=../lib/libintfc.a

INCLUDE =-I ./ -I$(INCDIR)

EXEC: ../bin/updatemac
	@echo "\nupdatemac MAKED!"

../bin/updatemac: $(OBJS)  $(LIBS) 
	@rm  -f ../bin/updatemac
	$(CCCC) ../bin/updatemac $(OBJS) $(LIBS)

$(OBJDIR)/updatemac.o:updatemac.c
	$(CC) $(OBJDIR)/updatemac.o updatemac.c
$(OBJDIR)/md5_dgst.o:md5_dgst.c
	$(CC) $(OBJDIR)/md5_dgst.o md5_dgst.c
$(OBJDIR)/md5.o:md5.c
	$(CC) $(OBJDIR)/md5.o md5.c
