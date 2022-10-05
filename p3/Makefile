all : s

s : s.c
	gcc -Wall -Werror -fsanitize=address s.c -o s -pthread

clean : 
	rm -f s
