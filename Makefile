# `li` is what we want to build and `li.cpp` is what's required to build it
li: src/li.cpp build/find.o build/highlight.o
	# -Wall: show all warnings
	# -Wextra -pedantic: more warnings
	g++ src/li.cpp build/find.o build/highlight.o -o li -Wall -Wextra -pedantic -std=c++11

build/find.o: src/find.cpp
	g++ -c src/find.cpp -o build/find.o -Wall -Wextra -pedantic -std=c++11

build/highlight.o: src/default_highlight.cpp
	g++ -c src/default_highlight.cpp -o build/highlight.o -Wall -Wextra -pedantic -std=c++11

clean:
	rm build/*.o
