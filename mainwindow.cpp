#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include "optiondialog.h"
#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkCylinderSource.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkProperty.h>
#include <vtkCamera.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->pushButton,  &QPushButton::released, this, &MainWindow::handleButton1);
    connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::handleButton2);

    connect(this, &MainWindow::statusUpdateMessage,
            ui->statusbar, &QStatusBar::showMessage);

    /* Create the model list */
    this->partList = new ModelPartList("Parts List");

    /* Link it to the treeview in the GUI */
    ui->treeView->setModel(this->partList);

    /* This needs adding to MainWindow constructor */
    /* Link a render window with the Qt widget */
    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    ui->widget->setRenderWindow(renderWindow);

    /* Add a renderer */
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

    /* Create an object and add to renderer (this will change later to display a CAD model) */
    /* Will just copy and paste cylinder example from before */
    // This creates a polygonal cylinder model with eight circumferential facets
    // (i.e, in practice an octagonal prism).
    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetResolution(8);

    // The mapper is responsible for pushing the geometry into the graphics
    // library. It may also do color mapping, if scalars or other attributes are defined.
    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

    // The actor is a grouping mechanism: besides the geometry (mapper), it
    // also has a property, transformation matrix, and/or texture map.
    // Here we set its color and rotate it around the X and Y axes.

    connect(ui->treeView, &QTreeView::clicked,
            this, &MainWindow::handleTreeClicked);

    ui->treeView->addAction(ui->actionItem_Options);


}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::handleButton1() {
    emit statusUpdateMessage(QString("Button 1 was clicked"), 0);
}

void MainWindow::handleButton2() {
    OptionDialog dialog(this);

    QModelIndex index = ui->treeView->currentIndex();
    if (index.isValid()) {
        ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());
        dialog.loadFromModelPart(selectedPart);
    }

    if (dialog.exec() == QDialog::Accepted) {
        QModelIndex index = ui->treeView->currentIndex();
        if (index.isValid()) {
            ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());
            dialog.saveToModelPart(selectedPart);
        }
        emit statusUpdateMessage(QString("Dialog accepted"), 0);
    } else {
        emit statusUpdateMessage(QString("Dialog rejected"), 0);
    }
}

void MainWindow::handleTreeClicked() {
    /* Get the index of the selected item */
    QModelIndex index = ui->treeView->currentIndex();

    /* Get a pointer to the item from the index */
    ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());

    /* Retrieve the name string from the item's data array */
    QString text = selectedPart->data(0).toString();

    emit statusUpdateMessage(QString("The selected item is: ") + text, 0);
}

void MainWindow::on_actionOpen_File_triggered() {
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open File"),
        "C:\\",
        tr("STL Files (*.stl);;Text Files (*.txt)"));

    if (!fileName.isEmpty()) {

        ModelPart* newPart = new ModelPart({ fileName, "true", "255", "255", "255" });

        // Check if an item is selected in the tree
        QModelIndex index = ui->treeView->currentIndex();

        if (index.isValid()) {
            // Add as child of selected item
            ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());
            selectedPart->appendChild(newPart);
        } else {
            // No selection - add to root
            partList->getRootItem()->appendChild(newPart);
        }

        partList->refreshModel();

        newPart->loadSTL(fileName);

        updateRender();

        emit statusUpdateMessage(QString("File opened: ") + fileName, 0);
    }
}

void MainWindow::on_actionItem_Options_triggered() {
    QModelIndex index = ui->treeView->currentIndex();

    if (!index.isValid()) {
        emit statusUpdateMessage(QString("No item selected"), 0);
        return;
    }

    ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());

    OptionDialog dialog(this);
    dialog.loadFromModelPart(selectedPart);

    if (dialog.exec() == QDialog::Accepted) {
        dialog.saveToModelPart(selectedPart);

        // Re-render to reflect changes
        updateRender();

        emit statusUpdateMessage(
            QString("Item updated: ") + selectedPart->data(0).toString(), 0);
    } else {
        emit statusUpdateMessage(QString("Edit cancelled"), 0);
    }
}

void MainWindow::updateRender() {
    renderer->RemoveAllViewProps();

    // Loop through ALL top-level items, not just the first one
    int rows = partList->rowCount(QModelIndex());
    for (int i = 0; i < rows; i++) {
        updateRenderFromTree(partList->index(i, 0, QModelIndex()));
    }

    renderer->ResetCamera();
    renderer->Render();
    ui->widget->update();  // Force the Qt widget to repaint
}

void MainWindow::updateRenderFromTree(const QModelIndex& index) {
    if (index.isValid()) {
        ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());

        // Only add actor if the part is visible
        if (selectedPart->visible()) {
            renderer->AddActor(selectedPart->getActor());
        }
    }

    if (!partList->hasChildren(index) || (index.flags() & Qt::ItemNeverHasChildren)) {
        return;
    }

    int rows = partList->rowCount(index);
    for (int i = 0; i < rows; i++) {
        updateRenderFromTree(partList->index(i, 0, index));
    }
}
