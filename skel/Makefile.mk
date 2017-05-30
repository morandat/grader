LDFLAGS+= # Les libs supplementaires
CFLAGS+= -Werror -Wextra -Wall -Wno-error=unused-parameter
ILLEGAL= # Une expression régulière compatible egrep interdisant certains noms de fonction
DEPS=provided.h enonce.c
OBJS=provided.o
