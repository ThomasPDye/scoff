objects = scoff.o main.o

scoff: $(objects)
	$(CC) -o $@ $^ -lavcodec -lavformat -lavutil -lswresample

.PHONY: all clean

all: scoff

clean:
	rm scoff $(objects)
