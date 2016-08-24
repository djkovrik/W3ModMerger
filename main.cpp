#include "mainwindow.h"
#include <QApplication>

// some stuff from stackoverflow
void myMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if(MainWindow::log == nullptr) {
        QByteArray localMsg = msg.toLocal8Bit();
        switch (type) {
            case QtDebugMsg:
                fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
                break;
            case QtWarningMsg:
                fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
                break;
            case QtCriticalMsg:
                fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
                break;
            case QtInfoMsg:
                fprintf(stderr, "Info: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
                break;
            case QtFatalMsg:
                fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
                abort();
        }
    }
    else {
        switch (type) {
            case QtDebugMsg:
            case QtWarningMsg:
            case QtCriticalMsg:
            case QtInfoMsg:
                if(MainWindow::log != nullptr) {
                    MainWindow::log->append(msg);
                }
                break;
            case QtFatalMsg:
                abort();
        }
    }
}

int main(int argc, char* argv[])
{
    qInstallMessageHandler(myMessageOutput);
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
