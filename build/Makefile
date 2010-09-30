##
## Made by texane <texane@gmail.com>
## 
## Started on Sun Oct 26 16:06:14 2008 texane
## Updated on Sun Oct 26 22:57:16 2008 texane
##



CC	:=	g++
LD	:=	g++
CFLAGS	:=	-Wall -O3 -I../src
LFLAGS	:=
RM	:=	rm -f


BIN	:=	kaaviz

SRCS	:=	../src/main.cc		\
		../src/bmp.cc

OBJS	:=	$(SRCS:.cc=.o)


all	:	$(BIN)
$(BIN)	:	$(OBJS)
		$(LD) $(LFLAGS) -o $(BIN) $(OBJS)
clean	:
		$(RM) $(OBJS)
fclean	:	clean
		$(RM) $(BIN)
re	:	fclean all
%.o	:	%.cc
		$(CC) $(CFLAGS) -o $@ -c $<
