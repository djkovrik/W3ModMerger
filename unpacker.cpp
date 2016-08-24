/**
 *  Compression types:
 *  0 - Not compressed
 *  1 - Zlib
 *  2 - Doboz ???
 *  3 - Doboz
 *  4 - Lz4 (not used)
 *  5 - Lz4
 */

#include "unpacker.h"

#include "libs/lz4.h"
#include "libs/doboz/Common.h"
#include "libs/doboz/Decompressor.h"
#include "libs/zlib/zstr.hpp"

#include <cstring>
#include <sstream>

void cat_stream(std::istream& is, std::ostream& os)
{
    const std::streamsize buff_size = 1 << 16;
    char * buff = new char [buff_size];
    while (true)
    {
        is.read(buff, buff_size);
        std::streamsize cnt = is.gcount();
        if (cnt == 0) break;
        os.write(buff, cnt);
    }
    delete [] buff;
}

void Unpacker::unpack(int algo, int sizec, int sizeu, char* buf_compressed, char* buf_uncompressed)
{
    doboz::Decompressor dd;
    std::stringstream input;
    std::stringstream output;
    zstr::istream zis(input);

    switch(algo) {
        case 0:
            std::memcpy(buf_uncompressed, buf_compressed, sizec);
            break;
        case 1:
            input.write(buf_compressed, sizec);
            cat_stream(zis, output);
            std::memcpy(buf_uncompressed, output.str().c_str(), sizeu);
            break;
        case 2:
        case 3:
            dd.decompress(buf_compressed, sizec, buf_uncompressed, sizeu);
            break;
        case 4:
        case 5:
            LZ4_decompress_safe(buf_compressed, buf_uncompressed, sizec, sizeu);
            break;

        default:
            break;
    }
}
