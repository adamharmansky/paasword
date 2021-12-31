all:
	cc paasword.c -lX11 -lcairo -lXinerama -lm -o paasword

run: all
	./paasword

install: all
	install paasword /usr/bin

clean:
	rm -f paasword
