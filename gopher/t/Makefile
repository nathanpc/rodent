.include "../common.mk"

# Flags
CFLAGS  += -I/usr/local/include
LDFLAGS += -L/usr/local/lib
LIBS    += -ltap

# Sources and Objects
TARGET  = test
OBJECTS := test.o 01_urlpar.o 02_urlgen.o gopher.o

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
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)
