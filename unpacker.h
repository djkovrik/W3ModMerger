#ifndef UNPACKER_H
#define UNPACKER_H

struct Unpacker
{
    void unpack(int algo, int sizec, int sizeu, char* buf_compressed, char* buf_uncompressed);
};

#endif // UNPACKER_H
