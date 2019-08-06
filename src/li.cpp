#include "li.h"

/*** data ***/
editorConfig E;

std::unordered_map<int, void(*)()> short_cuts({
    {CTRL_KEY('f'), editorFind}
});

/*** terminal ***/
void die(const char *s) {
    // `\x1b` is the escape. J command to clear screen
    write(STDOUT_FILENO, "\x1b[2J", 4);
    // H command to position the cursor
    write(STDOUT_FILENO, "\x1b[H", 3);
    perror(s);
    exit(1);
}

void disableRawMode() {
  if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &E.orig_termios) == -1)
    die("tcsetattr");
}

// the default terminal mode is canonical mode
// in which keyboard input is only sent to your program when `Enter` is pressed
// this allows `Backspace`, but does not works for more complex functionality
// what we need is raw mode
void enableRawMode() {
    if(tcgetattr(STDIN_FILENO, &E.orig_termios) == -1)
        die("tcgetattr");
    
    // register the disable function upon exit
    atexit(disableRawMode);

    struct termios raw = E.orig_termios;
    // close `ECHO` to prevent key from showing up
    // close `ICANON` to read byte by byte instead of line by line
    // close `ISIG` to turn off Ctrl-C and Ctrl-Z
    // close `IEXTEN` to turn off Ctrl-V
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    // close `IXON` to turn off Ctrl-S and Ctrl-Q
    // close `ICRNL` to turn off Ctrl-M
    // the other 3 are closed by default
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    // close `OPOST` to turn off "\n" to "\r\n"
    raw.c_oflag &= ~(OPOST);
    // closed by default
    raw.c_cflag &= ~(CS8);
    // minimum number of bytes before `read()` can return
    raw.c_cc[VMIN] = 0;
    // maximum amount of time to wait before `read()` returns
    // unit is 0.1 second
    raw.c_cc[VTIME] = 1;

    if(tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

// wait for one keypress and return
int editorReadKey() {
    int nread;
    char c;
    while((nread = read(STDIN_FILENO, &c, 1)) != 1) {
        // ignore EAGAIN to make the code suitable to Cygwin
        if (nread == -1 && errno != EAGAIN)
            die("read");
    }

    if (c == '\x1b') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';
        if (seq[0] == '[') {  
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                        case '7': return HOME_KEY;
                        case '8': return END_KEY;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        } else if (seq[0] == 'O') {
            switch (seq[1]) { // map arrows to wasd
                case 'H': return HOME_KEY;
                case 'F': return END_KEY;
            }
        }
        return '\x1b';
    }

    return c;
}

int getWindowSize(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
        return -1;
    } else {
        *cols = ws.ws_col;
        *rows = ws.ws_row;
        return 0;
    }
}

/*** row operation ***/
void editorUpdateRow(erow& row) {
    row.render = "";
    for (int j = 0; j < (int)row.chars.size(); j++) {
        if(row.chars[j] == '\t')
            row.render += std::string(LI_TAB, ' ');
        else
            row.render += row.chars[j];
    }
    E.dirty = true;
}

void editorInsertRow(int at, const std::string& s) {
    if (at < 0 || at > (int)E.rows.size()) return;
    if(at == (int)E.rows.size())
        E.rows.push_back(erow());
    else
        E.rows.insert(E.rows.begin() + at, erow());
    E.rows[at].chars = s;
    editorUpdateRow(E.rows[at]);
}

void editorDelRow(int at) {
    if (at < 0 || at >= (int)E.rows.size()) return;
    E.rows.erase(E.rows.begin() + at);
    E.dirty = true;
}

void editorRowInsertChar(erow& row, int at, int c) {
    if (at < 0 || at > (int)row.chars.size())
        at = row.chars.size();
    row.chars.insert(at, 1, c);
    editorUpdateRow(row);
}

void editorRowAppendString(erow& row, std::string& s) {
    row.chars += s;
    editorUpdateRow(row);
}

void editorRowDelChar(erow& row, int at) {
    if (at < 0 || at >= (int)row.chars.size()) return;
    row.chars.erase(at, 1);
    editorUpdateRow(row);
}

/*** editor operations ***/
void editorInsertChar(int c) {
    if (E.cy == (int)E.rows.size())
        editorInsertRow(E.rows.size(), "");
    editorRowInsertChar(E.rows[E.cy], E.cx, c);
    E.cx++;
}

void editorInsertNewline() {
    if (E.cx == 0) {
        editorInsertRow(E.cy, "");
    } else {
        editorInsertRow(E.cy+1, E.rows[E.cy].chars.substr(E.cx));
        E.rows[E.cy].chars = E.rows[E.cy].chars.substr(0, E.cx);
        editorUpdateRow(E.rows[E.cy]);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar() {
    if (E.cy == (int)E.rows.size()) return;
    if (E.cy == 0 && E.cx == 0) return;
    if (E.cx > 0) {
        editorRowDelChar(E.rows[E.cy], E.cx-1);
        E.cx--;
    } else {
        E.cx = E.rows[E.cy-1].chars.size();
        editorRowAppendString(E.rows[E.cy-1], E.rows[E.cy].chars);
        editorDelRow(E.cy);
        E.cy--;
    }
}

/*** file IO ***/
void editorOpen(const char *filename) {
    E.filename = std::string(filename);
    FILE *fp = fopen(filename, "r");
    if(!fp) 
        die("fopen");

    char *line = NULL;
    size_t linecap = 0;
    ssize_t linelen;
    while((linelen = getline(&line, &linecap, fp)) != -1) {
        while (linelen > 0 && (line[linelen - 1] == '\n' ||
                               line[linelen - 1] == '\r'))
            linelen--;
        editorInsertRow(E.rows.size(), std::string(line, linelen));
    }
    free(line);
    fclose(fp);
    E.dirty = false;
}

std::string editorRowsToString() {
    std::string buf;
    for(auto row : E.rows) {
        buf += row.chars + "\n";
    }
    return buf;
}

void editorSave() {
    if (E.filename == "") {
        E.filename = editorPrompt("Save as: ", nullptr);
        if (E.filename == "") {
            editorSetStatusMessage("Save aborted");
            return;
        }
    }
    std::string buf = editorRowsToString();
    int len = buf.size();
    // O_RDWR: read & write. O_CREAT: create if not exist. 
    // 0644 is the standard permission for text files
    int fd = open(E.filename.c_str(), O_RDWR | O_CREAT, 0644);
    if (fd != -1) {
        if(ftruncate(fd, len) != -1) { // set the file's size to specified length
            if(write(fd, buf.c_str(), len) == len) {
                close(fd);
                E.dirty = false;
                editorSetStatusMessage(std::to_string(len) + " bytes written to disk");
                return;
            }
        }
        close(fd);
    }
    editorSetStatusMessage("Can't save! I/O error: " + std::string(strerror(errno)));
}

/*** append buffer ***/
typedef std::string abuf;

abuf ABUF_INIT = "";

void abAppend(abuf& ab, std::string s) {
    ab += s;
}

/*** output ***/

void editorScroll() {
    if (E.cy < E.row_offset) {
        E.row_offset = E.cy;
    }
    if (E.cy >= E.row_offset + E.screenrows) {
        E.row_offset = E.cy - E.screenrows + 1;
    }
    if (E.cx < E.col_offset) {
        E.col_offset = E.cx;
    }
    if (E.cx >= E.col_offset + E.screencols) {
        E.col_offset = E.cx - E.screencols + 1;
    }
}

void editorDrawRows(abuf& ab) {
    for (int y = 0; y < E.screenrows; y++) {
        int filerow = y + E.row_offset;
        if (filerow >= (int)E.rows.size()) {
            if (E.rows.size() == 0 && y == E.screenrows / 3) {  // show welcome page
                std::string welcome = "li editor -- version " + LI_VERSION;
                welcome = "~" + std::string(std::max(0, (int)(E.screencols-welcome.size())/2), ' ') + 
                    welcome.substr(0, std::min((int)welcome.size(), E.screencols));
                abAppend(ab, welcome);
            } else {
                abAppend(ab, "~");
            }
        } else if ((int)E.rows[filerow].render.size() > E.col_offset) {
            std::string row = E.rows[filerow].render.substr(E.col_offset, 
                std::min((int)E.rows[filerow].render.size() - E.col_offset, E.screencols));
            if(highlight != nullptr)
                row = highlight(row);
            abAppend(ab, row);
        }
        // clear a line at a time
        abAppend(ab, "\x1b[K");
        // new line even for the last row, last row is status bar
        abAppend(ab, "\r\n");
    }
}

void editorDrawStatusBar(abuf& ab) {
    abAppend(ab, "\x1b[7m");
    std::string status = "  " + (E.filename == "" ? "[No Name]" : E.filename) + 
        " - " + std::to_string(E.rows.size()) + " lines" + 
        (E.dirty? " (modified)" : "");
    int len = status.size();
    abAppend(ab, status.substr(0, std::min(len, E.screencols)));
    std::string rstatus = std::to_string(E.cy+1) + "/" + std::to_string(E.rows.size()) + "  ";
    while (len < E.screencols) {
        if (E.screencols - len == (int)rstatus.size()) {
            abAppend(ab, rstatus);
            len += rstatus.size();
        } else {
            abAppend(ab, " ");
            len++;
        }
    }
    abAppend(ab, "\x1b[m");
    abAppend(ab, "\r\n");
}

void editorDrawMessageBar(abuf& ab) {
    // clear the bar
    abAppend(ab, "\x1b[K");
    abAppend(ab, E.status_msg);
}

void editorRefreshScreen() {
    editorScroll();

    abuf ab = ABUF_INIT;

    // hide the cursor before refreshing
    abAppend(ab, "\x1b[?25l");
    // `\x1b` is the escape. J command to clear screen
    // moved ti clear a line at a time in editorDrawRows
    // abAppend(ab, "\x1b[2J");
    // H command to position the cursor
    abAppend(ab, "\x1b[H");

    editorDrawRows(ab);
    editorDrawStatusBar(ab);
    editorDrawMessageBar(ab);

    abAppend(ab, "\x1b[" + std::to_string(E.cy-E.row_offset+1) + 
                     ";" + std::to_string(E.cx-E.col_offset+1) + "H");
    // show the cursor again immediately after the refresh
    abAppend(ab, "\x1b[?25h");

    write(STDOUT_FILENO, ab.c_str(), ab.size());
}

void editorSetStatusMessage(const std::string& msg) {
  E.status_msg = "  " + msg;
}

/*** input ***/

std::string editorPrompt(const std::string& prompt, void (*callback)(const std::string&, int)) {
    std::string buf = "";
    while(true) {
        editorSetStatusMessage(prompt + buf);
        editorRefreshScreen();

        int c = editorReadKey();
        if (c == '\x1b') {
            editorSetStatusMessage("");
            if (callback != nullptr)
                callback(buf, c);
            return "";
        } else if (c == DEL_KEY || c == CTRL_KEY('h') || c == BACKSPACE) {
            if (buf.size() != 0)
                buf.pop_back();
        } else if (c == '\r') {
            if (buf.size() != 0) {
                editorSetStatusMessage("");
                if (callback != nullptr)
                    callback(buf, c);
                return buf;
            }
        } else if (!iscntrl(c) && c < 128) {
            buf += c;
        }
        if (callback != nullptr)
                callback(buf, c);
    }
}

void editorMoveCursor(int key) {
    switch (key) {
        case ARROW_LEFT:
            if(E.cx)
                E.cx--;
            else if (E.cy > 0) {
                E.cy--;
                E.cx = E.rows[E.cy].render.size();
            }
            break;
        case ARROW_DOWN:
            E.cy = std::min(E.cy+1, (int)E.rows.size());
            break;
        case ARROW_RIGHT:
            if(E.cy != (int)E.rows.size() && E.cx < (int)E.rows[E.cy].render.size())
                E.cx++;
            else if (E.cy < (int)E.rows.size()) {
                E.cx = 0;
                E.cy++;
            }
            break;
        case ARROW_UP:
            E.cy = std::max(E.cy-1, 0);
            break;
    }
    if (E.cy != (int)E.rows.size())
        E.cx = std::min(E.cx, (int)E.rows[E.cy].render.size());
    else
        E.cx = 0;
}

// wait for a keypress and handle it
void editorProcessKeypress() {
    static int quit_times = LI_QUIT_TIMES;
    int c = editorReadKey();
    switch (c) {
        case '\r':  // use '\r' to get enter, don't know why...
            editorInsertNewline();
            break;
        case CTRL_KEY('c'):
            if(E.dirty && quit_times > 0) {
                editorSetStatusMessage("WARNING!!! File has unsaved changes. "
                    "Press Ctrl-C " + std::to_string(quit_times) + " more times to quit.");
                quit_times--;
                return;
            }
            // `\x1b` is the escape. J command to clear screen
            write(STDOUT_FILENO, "\x1b[2J", 4);
            // H command to position the cursor
            write(STDOUT_FILENO, "\x1b[H", 3);
            exit(0);
            break;
        case CTRL_KEY('s'):
            editorSave();
            break;
        case BACKSPACE:
        case CTRL_KEY('h'):
        case DEL_KEY:
            if (c == DEL_KEY) editorMoveCursor(ARROW_RIGHT);
                editorDelChar();
            break;
        case CTRL_KEY('l'):  // traditionally used to refresh
        case '\x1b':  // escape
            break;
        case ARROW_UP:
        case ARROW_DOWN:
        case ARROW_LEFT:
        case ARROW_RIGHT:
            editorMoveCursor(c);
            break;
        case HOME_KEY:
            E.cx = 0;
            break;
        case END_KEY:
            E.cx = E.rows[E.cy].render.size();
            break;
        case PAGE_UP:
        case PAGE_DOWN:
        {
            E.cy = c == PAGE_UP ? E.row_offset : std::min(E.row_offset + E.screenrows - 1, (int)E.rows.size());
            int times = E.screenrows;
            while (times--)
                editorMoveCursor(c == PAGE_UP ? ARROW_UP : ARROW_DOWN);
            break;
        }
        default:
            if (short_cuts.find(c) != short_cuts.end()) {
                short_cuts[c]();
                break;
            }
            editorInsertChar(c);
            break;
    }
    quit_times = LI_QUIT_TIMES;
}


/*** init ***/

void initEditor() {
    if(getWindowSize(&E.screenrows, &E.screencols) == -1)
        die("getWindowSize");
    E.screenrows -= 2;  // for status bar and message bar
    E.cx = 0;
    E.cy = 0;
    E.row_offset = 0;
    E.col_offset = 0;
    E.dirty = false;
}

int main(int argc, char *argv[]) {
    enableRawMode();
    initEditor();
    if (argc >= 2)
        editorOpen(argv[1]);

    editorSetStatusMessage("HELP: Ctrl-S = save | Ctrl-C = quit");

    while(1) {
        editorRefreshScreen();
        editorProcessKeypress();
    }
    return 0;
}