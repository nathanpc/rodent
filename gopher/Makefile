# Tools
CC  = gcc
GDB = gdb --args
RM  = rm -f

# Flags
CFLAGS  = -Wall --std=gnu89
LDFLAGS =
LIBS    =

# Sources and Objects
SOURCES = test.c gopher.c
TARGET   = rodent_test
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

.PHONY: all compile run debug memcheck clean
all: compile

compile: $(TARGET)	

run: $(TARGET)
	./$(TARGET)

debug: CFLAGS += -g3 -DDEBUG
debug: clean all

memcheck: CFLAGS += -g3 -DDEBUG -DMEMCHECK
memcheck: compile
	valgrind --tool=memcheck --leak-check=yes --show-leak-kinds=all \
		--track-origins=yes --log-file=valgrind.log ./$(TARGET)
	cat valgrind.log

clean:
	$(RM) valgrind.log
	$(RM) $(OBJECTS)
	$(RM) $(TARGET)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)
