# Project Info
This repository contains two versions of hangman solvers. Both solvers are implemented according to the same information-theoretic principles. Essentially, each subsequent guess is chosen such that the entropy of the potential world is decreased maximally. Note that if the chosen word or phrase is not present in the program wordlist (in words/ directory), the program will fail.

The first version is written in C, and the second version is written in Java.


# RUNNING HANGMAN (C VERSION)

Using make:
```
$ make
$ ./bin/hangman.exe
```

Using gcc:
```
$ gcc -c -o hangman.o hangman.c
$ gcc -o bin/hangman hangman.o -I/headers
$ ./bin/hangman.exe
```

# RUNNING HANGMAN (JAVA VERSION)

Requires Java JRE:
```
$ javac hangman.java
$ java hangman
```