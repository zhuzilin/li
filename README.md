# li (里)
li is another mini text editor with 500 loc and simple interfaces for short cut.

Inspired by [antirez/kilo](https://github.com/antirez/kilo) and the great tutorial [here](https://viewsourcecode.org/snaptoken/kilo/), li uses C++11 to simplify some pointer manipulation in kilo.

## Usage
To use li, just
```
> make
> li about_li
```
![about_li](https://github.com/zhuzilin/li/blob/master/img/about_li.png?raw=true)
## Add Short Cuts
Also to make li easier to customize, li extracted simple interfaces for short cut. The `find.cpp` is an example.
To add search for Ctrl+F, just claim the function needed in an external file like `find.cpp`:
```c++
// find.cpp
#include "li.h"

void editorFindCallback(const std::string& query, int key) {
    ...
}

void editorFind() {
    ...
}
```
And in `li.h` and `li.cpp`, add:
```c++
// li.h

/*** add-ons ***/
void editorFind();

// li.cpp

std::unordered_map<int, void(*)()> fns({
    {CTRL_KEY('f'), editorFind}
});
```
Last, add `find.cpp` to `Makefile` and make li. Now the search short cut has been added successfully!

## TODOs
- [ ] add modular highlight.

## Acknowledgement
- [antirez/kilo](https://github.com/antirez/kilo): A text editor in less than 1000 LOC with syntax highlight and search.
- [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/):  an instruction booklet that shows you how to build a text editor in C.
