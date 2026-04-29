#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModelIndex>

#include "ModelPart.h"
#include "ModelPartList.h"
#include "VRRenderThread.h"

#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

public slots:
    void handleButton2();
    void handleTreeClicked();

    void on_actionOpen_File_triggered();
    void on_actionItem_Options_triggered();

    void startVR();
    void stopVR();

signals:
    void statusUpdateMessage(const QString& message, int timeout);

private:
    Ui::MainWindow* ui;

    ModelPartList* partList;

    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;

    VRRenderThread* vrThread = nullptr;

    void updateRender();
    void updateRenderFromTree(const QModelIndex& index);

    int addPartToVRThread(const QModelIndex& index);
};

#endif // MAINWINDOW_H
