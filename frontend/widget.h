#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QJsonArray>

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_selectFileButton_clicked();

    void on_skipButton_clicked();

    void on_deleteLeftButton_clicked();

    void on_deleteRightButton_clicked();

private:
    void parseDuplicateFile(QString filename);
    void loadNextPair();
    void displayPhotoMetadata(QString leftFilename, QPixmap left, QString rightFilename, QPixmap right);

    Ui::Widget *ui;

    QJsonArray jsonDuplicateArray;
    int completedComparisons = 0;
    int totalComparisons = 0;
};
#endif // WIDGET_H
