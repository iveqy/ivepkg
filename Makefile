all:
	gcc -o ivepkg -Wall -std=gnu99 ivepkg.c db.c sqlite3.c -lssl -lpthread -ldl -lcrypto
clean:
	rm ivepkg
