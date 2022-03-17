#include "mainwindow.h"
#include "testworktimetracker.h"
#include "testhelper.h"

#include <QApplication>

void test()
{
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
