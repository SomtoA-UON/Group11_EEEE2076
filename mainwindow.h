#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ModelPart.h"
#include "ModelPartList.h"

#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
public slots:
    void handleButton1();
    void handleButton2();
    void handleTreeClicked();
    void on_actionOpen_File_triggered();
    void on_actionItem_Options_triggered();

private slots:
    void onTreeContextMenu(const QPoint& pos);
    void onEditFilters();
    void onChangeColour();

signals:
    void statusUpdateMessage(const QString& message, int timeout);
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    ModelPartList* partList;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow>renderWindow;
    void updateRender();
    void updateRenderFromTree(const QModelIndex& index);
};
#endif // MAINWINDOW_H
