#include "widget.h"
#include "ui_widget.h"
#include <QFileDialog>
#include <QDebug>

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    // Load placeholder images into labels
    QPixmap leftPlaceholderImage("://images/left.png");
    QPixmap rightPlaceholderImage("://images/right.png");

    ui->leftImage->setPixmap(leftPlaceholderImage);
    ui->leftImage->setScaledContents(true);
    ui->rightImage->setPixmap(rightPlaceholderImage);
    ui->rightImage->setScaledContents(true);
}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_selectFileButton_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "Select file for reading duplicates", ".");
    if (filename.isEmpty()) return;

    ui->statusLabel->setText(QString("Loading input from %1").arg(filename));
    ui->selectFileButton->setEnabled(false);
}
