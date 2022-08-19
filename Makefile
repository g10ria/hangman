compile=gcc
buildDir=bin
headersDir=headers

Hobj = hangman.o

%.o: %.c
	$(compile) -c -o $@ $<

hangman: $(Hobj)
	$(compile) -o ${buildDir}/$@ $^ -I/$(headersDir)