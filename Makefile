
dumpcard: dumpcard.o
	gcc $< -o $@

clean:
	rm -f dumpcard dumpcard.o

