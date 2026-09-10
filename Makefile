CC = cc
SRC = ./src/main.c

BUILDDIR = build
COMPILED = $(BUILDDIR)/app

RUN = ./$(COMPILED) 

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

it: | $(BUILDDIR)
	$(CC) $(SRC) -o $(COMPILED)

run: $(COMPILED)
	$(RUN)

clean: | $(BUILDDIR) 
	rm -r $(BUILDDIR)
