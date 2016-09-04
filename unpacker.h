#ifndef UNPACKER_H
#define UNPACKER_H

#include "singlemod.h"
#include "settings.h"

#include <QThread>

class Unpacker : public QObject
{
    Q_OBJECT

public:
    Unpacker(QList<Mod*>& list, const Settings* s);
    void run();
    void startUnpacking();

signals:
    void toLog(QString message);
    void finished();

private:
    QList<Mod*> modsList;
    const Settings* settings;
    void convertStream(std::istream& is, std::ostream& os);
    void unpack(int algo, int sizec, int sizeu, char* buf_compressed, char* buf_uncompressed);
};

#endif // UNPACKER_H
