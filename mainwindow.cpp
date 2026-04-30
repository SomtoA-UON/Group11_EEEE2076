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
#include <vtkLight.h>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    connect(ui->pushButton, &QPushButton::released, this, &MainWindow::handleButton1);
    connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::handleButton2);

    connect(this, &MainWindow::statusUpdateMessage,
        ui->statusbar, &QStatusBar::showMessage);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested,
        this, &MainWindow::onTreeContextMenu);

    /* Create the model list */
    this->partList = new ModelPartList("Parts List");

    /* Link it to the treeview in the GUI */
    ui->treeView->setModel(this->partList);

    /* Link a render window with the Qt widget */
    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    ui->widget->setRenderWindow(renderWindow);

    /* Add a renderer */
    renderer = vtkSmartPointer<vtkRenderer>::New();
    renderWindow->AddRenderer(renderer);

    /* --- FIXED LIGHTING --- */
    vtkSmartPointer<vtkLight> light = vtkSmartPointer<vtkLight>::New();
    light->SetLightTypeToHeadlight();   // Changed from SceneLight
    light->SetPosition(5, 5, 15);
    light->SetFocalPoint(0, 0, 0);
    light->SetIntensity(1.0);           // Increased intensity
    renderer->AddLight(light);
    /* --- END LIGHTING --- */

    /* Create cylinder */
    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetResolution(8);

    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

    /* --- ADDED ACTOR (CRITICAL FIX) --- */
    vtkNew<vtkActor> cylinderActor;
    cylinderActor->SetMapper(cylinderMapper);
    renderer->AddActor(cylinderActor);
    /* --- END ACTOR FIX --- */

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
    }
    else {
        emit statusUpdateMessage(QString("Dialog rejected"), 0);
    }
}

void MainWindow::handleTreeClicked() {
    QModelIndex index = ui->treeView->currentIndex();
    ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());
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

        QModelIndex index = ui->treeView->currentIndex();

        if (index.isValid()) {
            ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());
            selectedPart->appendChild(newPart);
        }
        else {
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
        updateRender();
        emit statusUpdateMessage(
            QString("Item updated: ") + selectedPart->data(0).toString(), 0);
    }
    else {
        emit statusUpdateMessage(QString("Edit cancelled"), 0);
    }
}

void MainWindow::updateRender() {
    renderer->RemoveAllViewProps();

    int rows = partList->rowCount(QModelIndex());
    for (int i = 0; i < rows; i++) {
        updateRenderFromTree(partList->index(i, 0, QModelIndex()));
    }

    renderer->ResetCamera();
    renderer->Render();
    ui->widget->update();
}

void MainWindow::updateRenderFromTree(const QModelIndex& index) {
    if (index.isValid()) {
        ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());

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

/*
void MainWindow::updateVRIfRunning(ModelPart* part) {
    if (m_vrThread && m_vrThread->isRunning()) {
        vtkActor* newVrActor = part->getNewActor();
        m_vrThread->updateActor(part->getVrActor(), newVrActor);
        part->setVrActor(newVrActor);
    }
}
*/

void MainWindow::onTreeContextMenu(const QPoint& pos) {
    qDebug() << "Context menu triggered at" << pos;

    QModelIndex index = ui->treeView->indexAt(pos);
    if (!index.isValid()) return; // right-clicked empty space

    QMenu menu(this);
    menu.addAction("Edit Filters", this, &MainWindow::onEditFilters);
    menu.addAction("Change Colour", this, &MainWindow::onChangeColour);
    menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
}

void MainWindow::onEditFilters() {
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) return;

    ModelPart* part = static_cast<ModelPart*>(index.internalPointer());
    if (!part) return;

    QDialog dialog(this);
    dialog.setWindowTitle("Edit Filters");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Clip filter
    QCheckBox* clipCheck = new QCheckBox("Apply Clip Filter", &dialog);
    clipCheck->setChecked(part->getClipEnabled());

    QLabel* clipLabel = new QLabel("Clip Position:", &dialog);
    QSlider* clipSlider = new QSlider(Qt::Horizontal, &dialog);
    clipSlider->setMinimum(-100);
    clipSlider->setMaximum(100);
    clipSlider->setValue(part->getClipOrigin());

    // Shrink filter
    QCheckBox* shrinkCheck = new QCheckBox("Apply Shrink Filter", &dialog);
    shrinkCheck->setChecked(part->getShrinkEnabled());

    QLabel* shrinkLabel = new QLabel("Shrink Factor:", &dialog);
    QSlider* shrinkSlider = new QSlider(Qt::Horizontal, &dialog);
    shrinkSlider->setMinimum(1);
    shrinkSlider->setMaximum(99);
    shrinkSlider->setValue(part->getShrinkFactor() * 100); // 0.01-0.99 stored as 1-99

    QPushButton* okButton = new QPushButton("OK", &dialog);
    connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    layout->addWidget(clipCheck);
    layout->addWidget(clipLabel);
    layout->addWidget(clipSlider);
    layout->addWidget(shrinkCheck);
    layout->addWidget(shrinkLabel);
    layout->addWidget(shrinkSlider);
    layout->addWidget(okButton);

    if (dialog.exec() == QDialog::Accepted) {
        part->setClipEnabled(clipCheck->isChecked());
        part->setShrinkEnabled(shrinkCheck->isChecked());
        part->setClipOrigin(clipSlider->value());
        part->setShrinkFactor(shrinkSlider->value() / 100.0);
        part->updatePipeline();
        ui->widget->renderWindow()->Render();
    }
}

void MainWindow::onChangeColour() {
    // TODO
}