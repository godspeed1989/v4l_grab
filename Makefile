CC=g++
CPPFLAGS=-Wall
TARGET=v4l_grab

all: $(TARGET)

test.o: test.cpp
$(TARGET).o: $(TARGET).cpp $(TARGET).h

$(TARGET): test.o v4l_grab.o
	$(CC) $+ -o $@

clean:
	rm -rf *.bmp *.yuv *.o

