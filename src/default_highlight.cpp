#include "li.h"

enum editorHighlight {
    HL_NORMAL = 39,
    HL_NUMBER = 31,
    HL_STRING = 35,
};

std::string default_highlight(const std::string& row) {
    std::string highlighted = "";
    bool string_flag = false;
    std::vector<uint8_t> hl(row.size(), HL_NORMAL);
    
    for(int i=0; i<(int)row.size(); i++) {
        char c = row[i];
        if(c == '"') {
            hl[i] = HL_STRING;
            string_flag = !string_flag;
        } else if (string_flag) {
            hl[i] = HL_STRING;
        } else if(isdigit(c)) {
            hl[i] = HL_NUMBER;
        }
    }
    int current_color = HL_NORMAL;
    for(int i=0; i<(int)row.size(); i++) {
        if(hl[i] != current_color) {
            current_color = hl[i];
            highlighted += "\x1b[" + std::to_string(hl[i]) + "m";
        }
        highlighted += row[i];
    }
    if (current_color != HL_NORMAL)
        highlighted += "\x1b[39m";
    return highlighted;
}

std::string(*highlight)(const std::string&) = default_highlight;