
CC = clang
CFLAGS = -Wall -Wextra -pedantic -std=c99
COMP_TARGET = dante@localhost
COMP_PORT = 7922

TARGET = pfjson
DEPS = status.c main.c
COPY = $(DEPS) Makefile

$(TARGET): $(DEPS) pfjson.h
	$(CC) -o $(TARGET) $(CFLAGS) $(DEPS)

remote:
	ssh $(COMP_TARGET) -p $(COMP_PORT) mkdir -p pfjson
	scp -P $(COMP_PORT) $(COPY) $(COMP_TARGET):pfjson
	ssh -t $(COMP_TARGET) -p $(COMP_PORT) "cd pfjson && make"
	ssh -t $(COMP_TARGET) -p $(COMP_PORT) "cd pfjson && doas ./pfjson"

clean:
	rm $(TARGET)
