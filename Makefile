OBJS = nefko_file.o     \
       nefko_image.o    \
       nefko_decrypt.o

GHETTO_PATH=../ghetto
GHETTO_INCLUDE=-I$(GHETTO_PATH)
GHETTO_LINK=-L$(GHETTO_PATH) -lghetto

INCLUDES = -I. -Wall $(GHETTO_INCLUDE)
DEFINES = -D_DEBUG

ENDIANESS = -DMACH_ENDIANESS=1

CC = gcc

CFLAGS = -O0 -g $(DEFINES) $(ENDIANESS) $(INCLUDES)
LDFLAGS = -shared $(GHETTO_LINK)

PHONY := clean tags cleantags

TARGET = libnefko.so

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $(TARGET) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(OBJS) $(TARGET)

tags: cleantags
	ctags -f tags *.h *.c $(GHETTO_PATH)/*.c $(GHETTO_PATH)/*.h

cleantags:
	rm tags
