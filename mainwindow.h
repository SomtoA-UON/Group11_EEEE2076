#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ModelPart.h"
#include "ModelPartList.h"

#include <QDoubleSpinBox>
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkLight.h>

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
    void onRemoveItem();

    /** Called when the light intensity slider value changes.
      * @param value Slider integer value (0-100), mapped to intensity 0.0-1.0.
      */
    void onLightIntensityChanged(int value);

    /** Called when any light position spinbox value changes.
      * Reads all three spinboxes and updates the light position.
      */
    void onLightPositionChanged();

signals:
    void statusUpdateMessage(const QString& message, int timeout);

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow* ui;
    ModelPartList* partList;

    vtkSmartPointer<vtkRenderer>               renderer;
    vtkSmartPointer<vtkGenericOpenGLRenderWindow> renderWindow;

    /** The single scene light whose intensity and position are user-adjustable. */
    vtkSmartPointer<vtkLight> m_light;

    /** Spinboxes for the X, Y, Z components of the light position.
      * Kept as members so onLightPositionChanged() can read all three.
      */
    QDoubleSpinBox* m_lightXSpin = nullptr;
    QDoubleSpinBox* m_lightYSpin = nullptr;
    QDoubleSpinBox* m_lightZSpin = nullptr;

    void updateRender();
    void updateRenderFromTree(const QModelIndex& index);

    /** Creates and docks the Lighting Controls panel.
      * Called once from the constructor.
      */
    void setupLightingDock();
};
#endif // MAINWINDOW_H