#include "BK2Loader.hpp"

#include <zip.h>


namespace movie {

auto loadBK2Movie(const std::string &path) -> BK2Movie {
    zip_t *file = zip_open(path.c_str(), ZIP_DEFAULT_COMPRESSION_LEVEL, 'r');
    zip_entry_open(file, "Input Log.txt");

    void *buf = nullptr;
    size_t size = zip_entry_size(file);
    size_t read = zip_entry_read(file, &buf, &size);

    //Manual parsing (ignoring the header thing)
    size_t index = 0;
    const char *str = (char*)buf;

    //Skip two lines
    int lines = 0;
    while(lines < 2) {
        if(str[index++] == '\n') {
            lines++;
        }
    }

    BK2Movie movie;

    while(true) {
        //Find fourth comma of the line
        int commas = 0;
        while(commas < 4) {
            if(str[index++] == ',') {
                commas++;
            }
        }

        //Get input |UP|DOWN|LEFT|RIGHT|START|SELECT|B|A|L|R|POWER|
        u16 input = 0;
        
        if(str[index++] != '.') { input |= (1 << 6); }
        if(str[index++] != '.') { input |= (1 << 7); }
        if(str[index++] != '.') { input |= (1 << 5); }
        if(str[index++] != '.') { input |= (1 << 4); }
        if(str[index++] != '.') { input |= (1 << 3); }
        if(str[index++] != '.') { input |= (1 << 2); }
        if(str[index++] != '.') { input |= (1 << 1); }
        if(str[index++] != '.') { input |= (1 << 0); }
        if(str[index++] != '.') { input |= (1 << 9); }
        if(str[index++] != '.') { input |= (1 << 8); }
        if(str[index++] != '.') { input |= (1 << 10); }

        movie.inputs.push_back(input);

        if(index >= (read - 100)) {
            break;
        }
    }

    free(buf);
    zip_close(file);

    printf("Frames: %zu\n", movie.inputs.size());

    return movie;
}

} //namespace movie