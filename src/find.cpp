#include "li.h"

void editorFindCallback(const std::string& query, int key) {
    // use static variable to save match position
    static int last_match_cy = -1;
    static int last_match_cx = 0;
    static int direction = 1;
    if (key == '\r' || key == '\x1b') {
        last_match_cy = -1;
        last_match_cx = 0;
        direction = 1;
        return;
    } else if (key == ARROW_DOWN || key == ARROW_RIGHT) {
        direction = 1;
    } else if (key == ARROW_UP || key == ARROW_LEFT) {
        direction = -1;
    } else {
        last_match_cy = -1;
    }
    if (last_match_cy == -1) {
        last_match_cx = 0;
        direction = 1;
    }
    int current_cy = last_match_cy;
    int current_cx = last_match_cx;
    for (int i = 0; i < (int)E.rows.size(); i++) {
        if (current_cy >= 0) {  // continue to search in the same row
            erow row = E.rows[current_cy];
            size_t match = row.chars.find(query, current_cx+1);
            if (match != std::string::npos) {
                last_match_cy = current_cy;
                last_match_cx = (int)match;
                E.cy = current_cy;
                E.cx = (int)match;
                if(E.cy - E.row_offset >= E.screenrows)
                    E.row_offset =  E.cy;
                if(E.cx - E.col_offset >= E.screencols)
                    E.col_offset =  E.cx;
                break;
            }
        }
        current_cy = (current_cy + direction + (int)E.rows.size()) % (int)E.rows.size();
        current_cx = 0;
        erow row = E.rows[current_cy];
        size_t match = row.chars.find(query, current_cx);
        if (match != std::string::npos) {
            last_match_cy = current_cy;
            last_match_cx = current_cx;
            E.cy = current_cy;
            E.cx = (int)match;
            if(E.cy - E.row_offset >= E.screenrows)
                E.row_offset =  E.cy;
            if(E.cx - E.col_offset >= E.screencols)
                E.col_offset =  E.cx;
            break;
        }
    }
}

void editorFind() {
    int saved_cx = E.cx;
    int saved_cy = E.cy;
    int saved_col_offset = E.col_offset;
    int saved_row_offset = E.row_offset;
    std::string query = editorPrompt("Search: ", editorFindCallback);
    if(query == "") { // return to old position
        E.cx = saved_cx;
        E.cy = saved_cy;
        E.col_offset = saved_col_offset;
        E.row_offset = saved_row_offset;
    }
}