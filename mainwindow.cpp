#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlTableModel>
#include <QSqlError>
#include <QDateTime>
#include "worktimetracker.h"
#include "testworktimetracker.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    auto db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(":memory:");

    Q_ASSERT(db.open());

    WorktimeTracker wt = TestWorktimeTracker::example(db);

    auto model = new QSqlTableModel(this);
    model->setTable("worktime");
    model->select();

    ui->tableView->setModel(model);
    ui->tableView->resizeColumnsToContents();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_worktimeBtn_clicked()
{
    auto model = new QSqlTableModel(this);
    model->setTable("worktime");
    model->select();

    ui->tableView->setModel(model);
    ui->tableView->resizeColumnsToContents();
}

void MainWindow::on_leavepassBtn_clicked()
{
    auto model = new QSqlTableModel(this);
    model->setTable("leavepass");
    model->select();

    ui->tableView->setModel(model);
    ui->tableView->resizeColumnsToContents();
}

void MainWindow::on_scheduleBtn_clicked()
{
    auto model = new QSqlTableModel(this);
    model->setTable("schedule");
    model->select();

    ui->tableView->setModel(model);
    ui->tableView->resizeColumnsToContents();
}

