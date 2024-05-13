cc=gcc
flags=-Wall -Werror
src=src
bin=bin

all: setup clean $(bin)/server $(bin)/client

setup:
	mkdir -p $(bin)

clean:
	rm -f $(bin)/*

$(bin)/server: $(src)/server.c $(src)/vector.c
	$(cc) $(flags) -o $@ $^

$(bin)/client: $(src)/client.c
	$(cc) $(flags) -o $@ $<