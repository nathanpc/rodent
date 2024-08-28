include ../common.mk

# Flags
CFLAGS  += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
LIBS    += /usr/local/lib/libtap.a

# Sources and Objects
SOURCES = test.c 01_urlpar.c 02_urlgen.c gopher.c
TARGET  = test
OBJECTS := $(patsubst %.c, %.o, $(SOURCES))

.PHONY: all compile run testcount debug memcheck clean
all: compile

compile: $(TARGET)

run: $(TARGET)
	./$(TARGET)

testcount: 01_url.c
	grep -o -E "\ttest_url[_\(a-z]+" $^ | wc -l

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
