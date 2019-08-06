# `li` is what we want to build and `li.cpp` is what's required to build it
li: src/li.cpp src/find.cpp
	# -Wall: show all warnings
	# -Wextra -pedantic: more warnings
	g++ src/li.cpp src/find.cpp -o li -Wall -Wextra -pedantic -std=c++11