#ifndef LI_H
#define LI_H

/*** includes ***/
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif
#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <ctype.h>     // for `iscntrl`
#include <stdio.h>     // for `printf`, `perror`
#include <unistd.h>    // for `read`
#include <termios.h>   // for turning on raw mode
#include <stdlib.h>    // for `atexit`, `exit`
#include <errno.h>     // for `errno`, `EAGAIN`
#include <sys/ioctl.h> // get window size
#include <fcntl.h>     // ftruncate
#include <string>
#include <string.h>
#include <vector>
#include <algorithm>    // std::min
#include <unordered_map>

/*** defines ***/
const std::string LI_VERSION = "0.0.1";
// CTRL_KEY will perform a bit-wise `and` with 00011111 
// (    set first 3 digit to 0) 
#define CTRL_KEY(k) ((k) & 0x1f)
const int LI_TAB  = 4;
const int LI_QUIT_TIMES = 1;

enum editorKey {
    BACKSPACE = 127,
    ARROW_LEFT  = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

struct erow {
    std::string chars;
    std::string render;
};

/*** data ***/

struct editorConfig {
    struct termios orig_termios;
    int screenrows;
    int screencols;
    int cx, cy;
    int row_offset;
    int col_offset;
    std::string filename;
    std::vector<erow> rows;
    std::string status_msg;
    bool dirty;
};

extern editorConfig E;

/*** prototypes ***/
void editorSetStatusMessage(const std::string& msg);
void editorRefreshScreen();
std::string editorPrompt(const std::string& prompt, void (*callback)(const std::string&, int));

/*** add-ons ***/
void editorFind();
extern std::unordered_map<int, void(*)()> short_cuts;

extern std::string(*highlight)(const std::string&);
#endif