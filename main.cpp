#include "mainwindow.h"
#include "testworktimetracker.h"
#include "testhelper.h"

#include <QApplication>

void test()
{
    // Comment argument -silent if you want to see
    // all test output and debug messages (qDebug)
    QStringList args = {"", "-silent"};

    TestHelper testHelper;
    QTest::qExec(&testHelper, args);

    TestWorktimeTracker testWorktimeTracker;
    QTest::qExec(&testWorktimeTracker, args);
}

int main(int argc, char *argv[])
{
    test();

    QApplication a(argc, argv);

    MainWindow w;
    w.show();
    return a.exec();
}
