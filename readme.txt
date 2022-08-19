Contents:
This repository contains battleship, hangman v1, and hangman v2.
Battleship and hangman v1 are written in C, and hangman v2 is written in Java.


----------RUNNING BATTLESHIP----------

Using make:
$ mingw32-make battleship
$ ./bin/battleship.exe

Alternatively, run the following commands:
$ gcc -c -o battleship.o battleship.c
$ gcc -c -o hashmap.o hashmap.c
$ gcc -c -o mt.o mt.c
$ gcc -o bin/battleship battleship.o hashmap.o mt.o -I/headers
$ ./bin/battleship.exe


----------RUNNING HANGMAN (C VERSION)----------

Using make:
$ mingw32-make hangman
$ ./bin/hangman.exe

Alternatively, run the following commands:
$ gcc -c -o hangman.o hangman.c
$ gcc -o bin/hangman hangman.o -I/headers
$ ./bin/hangman.exe


----------RUNNING HANGMAN (JAVA VERSION)----------

Requires Java JRE:
$ javac hangman.java
$ java hangman