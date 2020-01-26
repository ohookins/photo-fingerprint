#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QJsonArray>

// forward declarations
class QPixmap;

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

    // Loads an image from a given path and returns it
    // This wraps the logic that converts CR2 images to something Qt can understand.
    QPixmap loadPhoto(QString path) const;

    Ui::Widget *ui;

    QJsonArray jsonDuplicateArray;
    int completedComparisons = 0;
    int totalComparisons = 0;
};
#endif // WIDGET_H
