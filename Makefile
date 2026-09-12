CC = cc
SRC = ./src/main.c ./src/sucker.c

BUILDDIR = build
COMPILED = $(BUILDDIR)/app

RUN = ./$(COMPILED) 


$(BUILDDIR):
	mkdir -p $(BUILDDIR)

it: | $(BUILDDIR)
	$(CC) -Iinclude  $(SRC) -o $(COMPILED)

run: $(COMPILED)
	$(RUN)

clean: | $(BUILDDIR) 
	rm -r $(BUILDDIR)
