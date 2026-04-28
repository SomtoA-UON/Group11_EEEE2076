#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "OptionDialog.h"

#include <QFileDialog>
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
#include <vtkProperty.h>

/**
 * Constructor.
 */
MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    partList(nullptr),
    vrThread(nullptr)
{
    ui->setupUi(this);

    /*
     * Button connections.
     *
     * Your .ui file has:
     * - startVRButton
     * - pushButton_2
     */
    connect(ui->startVRButton,
        &QPushButton::released,
        this,
        &MainWindow::startVR);

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

    /*
     * Set a simple background colour.
     */
    renderer->SetBackground(0.75, 0.75, 0.75);

    emit statusUpdateMessage("Ready.", 3000);
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

/**
 * Start the VR renderer.
 *
 * This creates a VRRenderThread, asks each ModelPart for a new VR actor,
 * adds those actors to the VR thread, then starts the thread.
 */
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
        });

    connect(vrThread,
        &QThread::finished,
        vrThread,
        &QObject::deleteLater);

    vrThread->start();

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

/**
 * Handles tree item click.
 */
void MainWindow::handleTreeClicked()
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

    QString text = selectedPart->data(0).toString();

    emit statusUpdateMessage("The selected item is: " + text, 3000);
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
