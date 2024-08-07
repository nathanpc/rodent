.include "common.mk"

# Sources and Objects
TARGET  = rodent_test
OBJECTS := test.o gopher.o

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
	$(CC) $(CFLAGS) -o $@ $(OBJECTS) $(LDFLAGS) $(LIBS)
