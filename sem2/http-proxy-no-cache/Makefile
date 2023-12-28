CC = gcc
CFLAGS = -std=c11 -O3
TARGET = http_proxy
SOURCE_FILES = main.c proxy.h
LIBS = -lpthread

$(TARGET): $(SOURCE_FILES)
	$(CC) $(CFLAGS) $(SOURCE_FILES) -o $(TARGET) $(LIBS)

.PHONY: clean

clean:
	rm -f $(TARGET)
