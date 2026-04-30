#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QDockWidget>
#include <QDoubleSpinBox>
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
#include <QHBoxLayout>
#include <QFormLayout>
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

    /* ---- Scene light (stored as member so controls can modify it) ---- */
    m_light = vtkSmartPointer<vtkLight>::New();
    m_light->SetLightTypeToSceneLight();
    m_light->SetPosition(5.0, 5.0, 15.0);
    m_light->SetFocalPoint(0.0, 0.0, 0.0);
    m_light->SetIntensity(1.0);
    renderer->AddLight(m_light);
    /* ------------------------------------------------------------------ */

    /* Create placeholder cylinder */
    vtkNew<vtkCylinderSource> cylinder;
    cylinder->SetResolution(8);

    vtkNew<vtkPolyDataMapper> cylinderMapper;
    cylinderMapper->SetInputConnection(cylinder->GetOutputPort());

    vtkNew<vtkActor> cylinderActor;
    cylinderActor->SetMapper(cylinderMapper);
    renderer->AddActor(cylinderActor);

    connect(ui->treeView, &QTreeView::clicked,
        this, &MainWindow::handleTreeClicked);

    ui->treeView->addAction(ui->actionItem_Options);

    /* Build the lighting controls dock */
    setupLightingDock();
}

MainWindow::~MainWindow()
{
    delete ui;
}

/* --------------------------------------------------------------------------
 * Lighting dock
 * -------------------------------------------------------------------------- */

void MainWindow::setupLightingDock()
{
    QDockWidget* dock = new QDockWidget(tr("Lighting Controls"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea | Qt::BottomDockWidgetArea);

    QWidget* container = new QWidget(dock);
    QVBoxLayout* vbox = new QVBoxLayout(container);

    /* ---- Intensity slider ---- */
    QLabel* intensityLabel = new QLabel(tr("Light Intensity:"), container);
    QSlider* intensitySlider = new QSlider(Qt::Horizontal, container);
    intensitySlider->setMinimum(0);
    intensitySlider->setMaximum(100);
    /* Initialise to match the light's current intensity (1.0 → 100) */
    intensitySlider->setValue(static_cast<int>(m_light->GetIntensity() * 100.0));
    intensitySlider->setToolTip(tr("Adjust scene light intensity (0 – 100 %)"));

    connect(intensitySlider, &QSlider::valueChanged,
        this, &MainWindow::onLightIntensityChanged);

    vbox->addWidget(intensityLabel);
    vbox->addWidget(intensitySlider);

    /* ---- Position spinboxes ---- */
    QLabel* posLabel = new QLabel(tr("Light Position:"), container);
    vbox->addWidget(posLabel);

    QFormLayout* form = new QFormLayout();

    double pos[3];
    m_light->GetPosition(pos);                  // read initial position

    /* X */
    m_lightXSpin = new QDoubleSpinBox(container);
    m_lightXSpin->setRange(-200.0, 200.0);
    m_lightXSpin->setSingleStep(1.0);
    m_lightXSpin->setValue(pos[0]);
    m_lightXSpin->setToolTip(tr("Light X position"));
    form->addRow(tr("X:"), m_lightXSpin);

    /* Y */
    m_lightYSpin = new QDoubleSpinBox(container);
    m_lightYSpin->setRange(-200.0, 200.0);
    m_lightYSpin->setSingleStep(1.0);
    m_lightYSpin->setValue(pos[1]);
    m_lightYSpin->setToolTip(tr("Light Y position"));
    form->addRow(tr("Y:"), m_lightYSpin);

    /* Z */
    m_lightZSpin = new QDoubleSpinBox(container);
    m_lightZSpin->setRange(-200.0, 200.0);
    m_lightZSpin->setSingleStep(1.0);
    m_lightZSpin->setValue(pos[2]);
    m_lightZSpin->setToolTip(tr("Light Z position"));
    form->addRow(tr("Z:"), m_lightZSpin);

    vbox->addLayout(form);

    connect(m_lightXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::onLightPositionChanged);
    connect(m_lightYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::onLightPositionChanged);
    connect(m_lightZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
        this, &MainWindow::onLightPositionChanged);

    vbox->addStretch();
    container->setLayout(vbox);
    dock->setWidget(container);

    addDockWidget(Qt::RightDockWidgetArea, dock);
}

/* --------------------------------------------------------------------------
 * Lighting slots
 * -------------------------------------------------------------------------- */

 /** Converts the integer slider value (0-100) to a 0.0-1.0 intensity and
   * applies it to the scene light, then re-renders.
   */
void MainWindow::onLightIntensityChanged(int value)
{
    double intensity = value / 100.0;
    m_light->SetIntensity(intensity);
    renderWindow->Render();
    ui->widget->update();
    emit statusUpdateMessage(
        QString("Light intensity set to %1%").arg(value), 2000);
}

/** Reads the X, Y, Z spinboxes and updates the light position, then re-renders. */
void MainWindow::onLightPositionChanged()
{
    double x = m_lightXSpin->value();
    double y = m_lightYSpin->value();
    double z = m_lightZSpin->value();
    m_light->SetPosition(x, y, z);
    renderWindow->Render();
    ui->widget->update();
    emit statusUpdateMessage(
        QString("Light position: (%1, %2, %3)").arg(x).arg(y).arg(z), 2000);
}

/* --------------------------------------------------------------------------
 * Existing slots (unchanged)
 * -------------------------------------------------------------------------- */

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

void MainWindow::onTreeContextMenu(const QPoint& pos) {
    qDebug() << "Context menu triggered at" << pos;

    QModelIndex index = ui->treeView->indexAt(pos);
    if (!index.isValid()) return;

    QMenu menu(this);
    menu.addAction("Edit Filters", this, &MainWindow::onEditFilters);
    menu.addAction("Change Colour", this, &MainWindow::onChangeColour);
    menu.addAction("Remove Item", this, &MainWindow::onRemoveItem);
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

    QCheckBox* clipCheck = new QCheckBox("Apply Clip Filter", &dialog);
    clipCheck->setChecked(part->getClipEnabled());

    QLabel* clipLabel = new QLabel("Clip Position:", &dialog);
    QSlider* clipSlider = new QSlider(Qt::Horizontal, &dialog);
    clipSlider->setMinimum(-100);
    clipSlider->setMaximum(100);
    clipSlider->setValue(part->getClipOrigin());

    QCheckBox* shrinkCheck = new QCheckBox("Apply Shrink Filter", &dialog);
    shrinkCheck->setChecked(part->getShrinkEnabled());

    QLabel* shrinkLabel = new QLabel("Shrink Factor:", &dialog);
    QSlider* shrinkSlider = new QSlider(Qt::Horizontal, &dialog);
    shrinkSlider->setMinimum(1);
    shrinkSlider->setMaximum(99);
    shrinkSlider->setValue(part->getShrinkFactor() * 100);

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

void MainWindow::onRemoveItem() {
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) {
        emit statusUpdateMessage(QString("No item selected to remove"), 0);
        return;
    }

    ModelPart* part = static_cast<ModelPart*>(index.internalPointer());
    QString name = part->data(0).toString();

    partList->removeItem(index);
    updateRender();

    emit statusUpdateMessage(QString("Removed: ") + name, 0);
}