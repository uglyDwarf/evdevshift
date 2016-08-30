CPPFLAGS=-g -Wall -Wextra
COMMON_SOURCES=ops.c parser.c ev_process.c
PARSER_SOURCES=config_bison.c config_lex.c
SOURCES_EDS=$(COMMON_SOURCES) evdevshift.c
SOURCES_SC1=$(COMMON_SOURCES) scafold1.c

OBJECTS_EDS=$(SOURCES_EDS:.c=.o) $(PARSER_SOURCES:.c=.o)
EXECUTABLE=evdevshift

all: $(EXECUTABLE)

clean:
	rm -f *.o $(EXECUTABLE) scafold1 config_bison.* config_lex.c *.gcov *.gcda *.gcno

$(EXECUTABLE): $(OBJECTS_EDS)

$(SOURCES_EDS): $(PARSER_SOURCES)

config_bison.c config_bison.h : config.y
	bison -r all -d -p eds -o config_bison.c config.y

config_lex.c : config.l config_bison.h
	flex -o $@ -P eds $<

test: scafold1
	./scafold1 test.conf
	gcov ev_process.c

scafold1: $(SOURCES_SC1) $(PARSER_SOURCES)
	gcc -o $@ -g -fprofile-arcs -ftest-coverage $^
