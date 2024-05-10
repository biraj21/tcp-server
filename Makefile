cc=gcc
flags=-Wall -Werror
src=src
bin=bin

all: clean $(bin)/server $(bin)/client

clean:
	rm -f $(bin)/*

$(bin)/server: $(src)/server.c
	$(cc) $(flags) -o $@ $<

$(bin)/client: $(src)/client.c
	$(cc) $(flags) -o $@ $<