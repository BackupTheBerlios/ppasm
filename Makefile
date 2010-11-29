# i benutze normaleweise das, damit es schneller geht
#TMPDIR=/dev/shm
TMPDIR=.
CC=gcc
CFLAGS=-g -c -std=c99
LD=gcc
LDFLAGS=
EXECUTABLE=ppasm
SOURCES=assemble.c expression.c opcodes.c parse.c stringext.c util.c loader.c main.c test.c
OBJECTS=$(SOURCES:.c=.o)

#------------------------------------------------------------------------------
.SUFFIXES:
.SUFFIXES: .c .o

%.o: %.c
	@echo compiling \"$<\"
	@$(CC) $(CFLAGS) $< -o $(TMPDIR)/$@

$(EXECUTABLE): $(OBJECTS)
	@echo linking \"$@\"
	@cd $(TMPDIR) && $(LD) $(LDFLAGS) $(OBJECTS) -o $@ && chmod +x $(EXECUTABLE)
	@echo creating a symlink to \"$@\"
	@if [ ! -e ./$(EXECUTABLE) ] ;  then ln -s $(TMPDIR)/$(EXECUTABLE) ./$(EXECUTABLE)  ;	fi

all: $(SOURCES) $(EXECUTABLE)

clean:
	cd $(TMPDIR) ; rm $(EXECUTABLE) $(OBJECTS)

