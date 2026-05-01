#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "OptionDialog.h"

#include <QFileDialog>
#include <QColorDialog>
#include <QDockWidget>
#include <QDoubleSpinBox>
#include "optiondialog.h"
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTreeView>

#include <vtkSmartPointer.h>
#include <vtkGenericOpenGLRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkLight.h>
#include <QCheckBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QSlider>
#include <QLabel>
#include <vtkProperty.h>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    partList(nullptr),
    vrThread(nullptr)
{
    ui->setupUi(this);
    connect(ui->startVRButton, &QPushButton::released, this, &MainWindow::startVR);
    connect(ui->pushButton_2, &QPushButton::released, this, &MainWindow::handleButton2);

    connect(this, &MainWindow::statusUpdateMessage,
        ui->statusbar, &QStatusBar::showMessage);

    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui->treeView, &QTreeView::customContextMenuRequested,
        this, &MainWindow::onTreeContextMenu);

    connect(ui->stopVRButton,
        &QPushButton::released,
        this,
        &MainWindow::stopVR);

    ui->stopVRButton->setEnabled(false);

    connect(ui->pushButton_2,
        &QPushButton::released,
        this,
        &MainWindow::handleButton2);

    /*
     * Status bar message connection.
     */
    connect(this,
        &MainWindow::statusUpdateMessage,
        ui->statusbar,
        &QStatusBar::showMessage);

    /*
     * Create the model list.
     */
    partList = new ModelPartList("Parts List");

    /*
     * Link the model list to the tree view.
     */
    ui->treeView->setModel(partList);

    /*
     * Connect tree click signal.
     */
    connect(ui->treeView,
        &QTreeView::clicked,
        this,
        &MainWindow::handleTreeClicked);

    /*
     * Allow the item options action to be used from the tree view.
     */
    ui->treeView->addAction(ui->actionItem_Options);

    /*
     * Create the VTK render window and attach it to the Qt VTK widget.
     */

    renderWindow = vtkSmartPointer<vtkGenericOpenGLRenderWindow>::New();
    ui->widget->setRenderWindow(renderWindow);

    /*
     * Create renderer.
     */
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

    renderer->SetBackground(0, 0, 0);

    emit statusUpdateMessage("Ready.", 3000);

    connect(ui->treeView, &QTreeView::clicked,
        this, &MainWindow::handleTreeClicked);

    ui->treeView->addAction(ui->actionItem_Options);

    /* Build the lighting controls dock */
    setupLightingDock();
}

/**
 * Destructor.
 */
MainWindow::~MainWindow()
{
    /*
     * Stop VR safely if it is still running.
     */
    if (vrThread != nullptr && vrThread->isRunning())
    {
        vrThread->issueCommand(VRRenderThread::END_RENDER, 0.0);
        vrThread->wait();
    }

    delete partList;
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

 /**
  * Start the VR renderer.
  *
  * This creates a VRRenderThread, asks each ModelPart for a new VR actor,
  * adds those actors to the VR thread, then starts the thread.
  */
void MainWindow::stopVR()
{
    if (vrThread == nullptr || !vrThread->isRunning())
    {
        emit statusUpdateMessage("VR is not running.", 3000);
        return;
    }

    vrThread->issueCommand(VRRenderThread::END_RENDER, 0.0);

    ui->stopVRButton->setEnabled(false);

    emit statusUpdateMessage("Stopping VR...", 3000);
}
void MainWindow::startVR()
{
    if (vrThread != nullptr && vrThread->isRunning())
    {
        emit statusUpdateMessage("VR is already running.", 3000);
        return;
    }

    if (partList == nullptr || partList->rowCount(QModelIndex()) == 0)
    {
        emit statusUpdateMessage("Load an STL model before starting VR.", 3000);
        return;
    }


    vrThread = new VRRenderThread(this);

    int actorCount = 0;

    int rows = partList->rowCount(QModelIndex());

    for (int i = 0; i < rows; i++)
    {
        QModelIndex index = partList->index(i, 0, QModelIndex());
        actorCount += addPartToVRThread(index);
    }

    if (actorCount == 0)
    {
        delete vrThread;
        vrThread = nullptr;

        emit statusUpdateMessage("No visible STL actors found for VR.", 3000);
        return;
    }

    connect(vrThread,
        &QThread::finished,
        this,
        [this]()
        {
            emit statusUpdateMessage("VR stopped.", 3000);

            vrThread = nullptr;

            ui->startVRButton->setEnabled(true);
            ui->stopVRButton->setEnabled(false);
        });

    connect(vrThread,
        &QThread::finished,
        vrThread,
        &QObject::deleteLater);

    vrThread->start();

    ui->startVRButton->setEnabled(false);
    ui->stopVRButton->setEnabled(true);

    emit statusUpdateMessage("VR started.", 3000);
}

/**
 * Recursively adds visible model parts to the VR thread.
 *
 * Each ModelPart creates a separate VR actor using getNewActor().
 * The GUI actor is not reused.
 */
int MainWindow::addPartToVRThread(const QModelIndex& index)
{
    if (!index.isValid() || vrThread == nullptr)
    {
        return 0;
    }

    int actorCount = 0;

    ModelPart* selectedPart =
        static_cast<ModelPart*>(index.internalPointer());

    if (selectedPart != nullptr && selectedPart->visible())
    {
        vtkSmartPointer<vtkActor> vrActor = selectedPart->getNewActor();

        if (vrActor != nullptr)
        {
            vrThread->addActorOffline(vrActor.GetPointer());
            actorCount++;
        }
    }

    int rows = partList->rowCount(index);

    for (int i = 0; i < rows; i++)
    {
        QModelIndex childIndex = partList->index(i, 0, index);
        actorCount += addPartToVRThread(childIndex);
    }

    return actorCount;
}

/**
 * Opens the item options dialog from pushButton_2.
 */
void MainWindow::handleButton2()
{
    onChangeColour();
}

/**
 * Handles tree item click.
 */
void MainWindow::handleTreeClicked()
{
    QModelIndex index = ui->treeView->currentIndex();
    ModelPart* selectedPart = static_cast<ModelPart*>(index.internalPointer());
    QString text = selectedPart->data(0).toString();
    emit statusUpdateMessage(QString("The selected item is: ") + text, 0);

    int rows = partList->rowCount(QModelIndex());
    for (int i = 0; i < rows; i++) {
        updateRenderFromTree(partList->index(i, 0, QModelIndex()));
    }

    renderer->ResetCamera();
    renderer->Render();
    ui->widget->update();
    if (!index.isValid())
    {
        emit statusUpdateMessage("No item selected.", 3000);
    }
    return;
}

/**
 * Open an STL file and add it to the model tree.
 */
void MainWindow::on_actionOpen_File_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Open File"),
        "C:\\",
        tr("STL Files (*.stl)")
    );

    if (fileName.isEmpty())
    {
        emit statusUpdateMessage("Open file cancelled.", 3000);
        return;
    }

    QFileInfo fileInfo(fileName);

    /*
     * Use only the filename in the tree view, not the full path.
     */
    ModelPart* newPart = new ModelPart({
        fileInfo.fileName(),
        "true",
        "255",
        "255",
        "255"
        });

    /*
     * Load STL before rendering.
     */
    newPart->loadSTL(fileName);

    /*
     * Add as a child of selected item, or to root if nothing is selected.
     */
    QModelIndex index = ui->treeView->currentIndex();

    if (index.isValid())
    {
        ModelPart* selectedPart =
            static_cast<ModelPart*>(index.internalPointer());

        if (selectedPart != nullptr)
        {
            selectedPart->appendChild(newPart);
        }
        else
        {
            partList->getRootItem()->appendChild(newPart);
        }
    }
    else
    {
        partList->getRootItem()->appendChild(newPart);
    }

    partList->refreshModel();

    updateRender();

    emit statusUpdateMessage("File opened: " + fileInfo.fileName(), 3000);
}

/**
 * Opens the item options dialog from the menu/action.
 */
void MainWindow::on_actionItem_Options_triggered()
{
    QModelIndex index = ui->treeView->currentIndex();

    if (!index.isValid())
    {
        emit statusUpdateMessage("No item selected.", 3000);
        return;
    }

    ModelPart* selectedPart =
        static_cast<ModelPart*>(index.internalPointer());

    if (selectedPart == nullptr)
    {
        emit statusUpdateMessage("Invalid item selected.", 3000);
        return;
    }

    OptionDialog dialog(this);
    dialog.loadFromModelPart(selectedPart);

    if (dialog.exec() == QDialog::Accepted)
    {
        dialog.saveToModelPart(selectedPart);

        updateRender();

        emit statusUpdateMessage("Item updated: " +
            selectedPart->data(0).toString(),
            3000);
    }
    else
    {
        emit statusUpdateMessage("Edit cancelled.", 3000);
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
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid()) {
        emit statusUpdateMessage("No item selected.", 3000);
        return;
    }

    ModelPart* part = static_cast<ModelPart*>(index.internalPointer());
    if (!part) return;

    /* Open colour picker pre-filled with the part's current colour */
    QColor initial(part->getColourR(), part->getColourG(), part->getColourB());
    QColor chosen = QColorDialog::getColor(initial, this, "Choose Part Colour");

    if (!chosen.isValid())
        return;   /* User cancelled */

    /* Apply the chosen colour to the model part and its VTK actor */
    part->setColour(
        static_cast<unsigned char>(chosen.red()),
        static_cast<unsigned char>(chosen.green()),
        static_cast<unsigned char>(chosen.blue())
    );

    /* Refresh the tree view so the colour swatch updates */
    partList->refreshModel();

    /* Re-render the VTK viewport */
    updateRender();

    emit statusUpdateMessage(
        QString("Colour changed to R:%1 G:%2 B:%3")
        .arg(chosen.red()).arg(chosen.green()).arg(chosen.blue()),
        3000);
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
/**
 * Rebuilds the normal Qt/VTK render view from the tree.
 */
void MainWindow::updateRender()
{
    if (renderer == nullptr)
    {
        return;
    }

    renderer->RemoveAllViewProps();

    int rows = partList->rowCount(QModelIndex());

    for (int i = 0; i < rows; i++)
    {
        QModelIndex index = partList->index(i, 0, QModelIndex());
        updateRenderFromTree(index);
    }

    renderer->ResetCamera();

    renderWindow->Render();
    ui->widget->update();
}

/**
 * Recursively adds visible actors from the tree to the normal Qt/VTK renderer.
 */
void MainWindow::updateRenderFromTree(const QModelIndex& index)
{
    if (!index.isValid())
    {
        return;
    }

    ModelPart* selectedPart =
        static_cast<ModelPart*>(index.internalPointer());

    if (selectedPart != nullptr && selectedPart->visible())
    {
        vtkSmartPointer<vtkActor> actor = selectedPart->getActor();

        if (actor != nullptr)
        {
            renderer->AddActor(actor);
        }
    }

    int rows = partList->rowCount(index);

    for (int i = 0; i < rows; i++)
    {
        QModelIndex childIndex = partList->index(i, 0, index);
        updateRenderFromTree(childIndex);
    }
}