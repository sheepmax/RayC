OBJS = main.c 

OBJ_NAME = main

all : $(OBJS)
	gcc $(OBJS) -o $(OBJ_NAME) -LC:\MinGW\lib -IC:\MinGW\include -lmingw32 -lSDL2main -lSDL2 -lpng