all:
	make -C ./src
	cp ./src/clichat .
	cp ./src/srvchat .
	cp ./src/testchat .

clean:
	make -C ./src clean
	rm -f clichat srvchat testchat
