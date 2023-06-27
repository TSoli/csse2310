all: sigcat hq
sigcat: sigcat.c
	gcc -g -Wall -pedantic -std=gnu99 \
	    -I/local/courses/csse2310/include \
	    -L/local/courses/csse2310/lib \
	    -lcsse2310a3 -o $@ $<
hq: hq.c
	gcc -g -Wall -pedantic -std=gnu99 \
	    -I/local/courses/csse2310/include \
	    -L/local/courses/csse2310/lib \
	    -lcsse2310a3 -o $@ $<
clean:
	rm sigcat
	rm hq
