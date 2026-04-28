/**
 * @file VRRenderThread.cpp
 *
 * EEEE2076 - Software Development Group Design Project
 *
 * VR rendering thread for the Qt/VTK application.
 */

#include "VRRenderThread.h"

 /* Qt headers */
#include <QCoreApplication>
#include <QFileInfo>
#include <QMutexLocker>
#include <QString>

/* Standard headers */
#include <array>

/* VTK headers */
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkLight.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOpenVRCamera.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkProperty.h>

/**
 * Constructor.
 *
 * This runs in the main Qt GUI thread. The actual VR renderer does not start
 * until VRRenderThread::start() is called, which then runs VRRenderThread::run().
 */
VRRenderThread::VRRenderThread(QObject* parent)
    : QThread(parent)
{
    actors = vtkSmartPointer<vtkActorCollection>::New();

    endRender = false;

    rotateX = 0.0;
    rotateY = 0.0;
    rotateZ = 0.0;
}

/**
 * Destructor.
 *
 * Tries to stop the VR thread safely if it is still running.
 */
VRRenderThread::~VRRenderThread()
{
    issueCommand(END_RENDER, 0.0);

    if (interactor != nullptr)
    {
        interactor->TerminateApp();
    }

    if (window != nullptr)
    {
        window->Finalize();
    }

    if (isRunning())
    {
        wait();
    }
}

/**
 * Adds an actor to the VR scene before the VR thread starts.
 *
 * The coursework brief says these actors must not be the same actors used in
 * the Qt renderer. Each ModelPart should create a new actor and mapper for VR.
 */
void VRRenderThread::addActorOffline(vtkActor* actor)
{
    if (actor == nullptr)
    {
        return;
    }

    /*
     * Only allow actors to be added before the thread starts.
     * Once the VR renderer is running, VTK objects should not be changed from
     * the GUI thread because VTK is not generally thread-safe.
     */
    if (!isRunning())
    {
        QMutexLocker locker(&mutex);

        double* actorOrigin = actor->GetOrigin();

        /*
         * These transforms come from the template idea.
         * They may need adjusting depending on your STL model scale/position.
         */
        actor->RotateX(-90.0);
        actor->AddPosition(
            -actorOrigin[0] + 0.0,
            -actorOrigin[1] - 100.0,
            -actorOrigin[2] - 200.0
        );

        actors->AddItem(actor);
    }
}

/**
 * Sends commands from the GUI thread to the VR thread.
 *
 * Example:
 * vrThread->issueCommand(VRRenderThread::ROTATE_Z, 1.0);
 */
void VRRenderThread::issueCommand(int cmd, double value)
{
    QMutexLocker locker(&mutex);

    switch (cmd)
    {
    case END_RENDER:
        endRender = true;
        break;

    case ROTATE_X:
        rotateX = value;
        break;

    case ROTATE_Y:
        rotateY = value;
        break;

    case ROTATE_Z:
        rotateZ = value;
        break;

    default:
        break;
    }
}

/**
 * Main VR render thread.
 *
 * This function runs separately from the main Qt GUI thread.
 */
void VRRenderThread::run()
{
    /*
     * Create a background colour.
     */
    vtkNew<vtkNamedColors> colors;

    std::array<unsigned char, 4> backgroundColour{ {26, 51, 102, 255} };
    colors->SetColor("BkgColor", backgroundColour.data());

    /*
     * Create the VR renderer.
     */
    renderer = vtkSmartPointer<vtkOpenVRRenderer>::New();
    renderer->SetBackground(colors->GetColor3d("BkgColor").GetData());

    /*
     * Add all actors that were added using addActorOffline().
     */
    {
        QMutexLocker locker(&mutex);

        vtkActor* actor = nullptr;
        actors->InitTraversal();

        while ((actor = actors->GetNextActor()) != nullptr)
        {
            renderer->AddActor(actor);
        }
    }

    /*
     * Create the OpenVR render window.
     */
    window = vtkSmartPointer<vtkOpenVRRenderWindow>::New();

    /*
     * Tell VTK where the OpenVR action/binding JSON files are.
     *
     * This expects the JSON files to be copied next to the .exe.
     * If they are inside a vrbindings folder next to the .exe, this also handles that.
     */
    QString actionManifestDirectory = QCoreApplication::applicationDirPath();

    if (!QFileInfo::exists(actionManifestDirectory + "/vtk_openvr_actions.json") &&
        QFileInfo::exists(actionManifestDirectory + "/vrbindings/vtk_openvr_actions.json"))
    {
        actionManifestDirectory += "/vrbindings";
    }

    window->SetActionManifestDirectory(actionManifestDirectory.toStdString());

    window->Initialize();
    window->AddRenderer(renderer);

    /*
     * Create the OpenVR camera.
     */
    camera = vtkSmartPointer<vtkOpenVRCamera>::New();
    renderer->SetActiveCamera(camera);

    /*
     * Add a simple scene light.
     */
    vtkNew<vtkLight> light;
    light->SetLightTypeToSceneLight();
    light->SetPosition(5.0, 5.0, 15.0);
    light->SetPositional(true);
    light->SetConeAngle(30.0);
    light->SetFocalPoint(0.0, 0.0, 0.0);
    light->SetDiffuseColor(1.0, 1.0, 1.0);
    light->SetAmbientColor(1.0, 1.0, 1.0);
    light->SetSpecularColor(1.0, 1.0, 1.0);
    light->SetIntensity(0.8);
    renderer->AddLight(light);

    /*
     * Create the OpenVR interactor.
     */
    interactor = vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New();
    interactor->SetRenderWindow(window);
    interactor->Initialize();

    renderer->ResetCamera();
    window->Render();

    /*
     * Start the manual VR event loop.
     *
     * This manual loop allows the GUI thread to send commands such as:
     * - stop VR
     * - rotate model
     * - later, update filters/colours/etc.
     */
    {
        QMutexLocker locker(&mutex);
        endRender = false;
    }

    t_last = std::chrono::steady_clock::now();

    while (!interactor->GetDone())
    {
        double localRotateX = 0.0;
        double localRotateY = 0.0;
        double localRotateZ = 0.0;
        bool localEndRender = false;

        /*
         * Copy command values safely from shared variables.
         */
        {
            QMutexLocker locker(&mutex);

            localRotateX = rotateX;
            localRotateY = rotateY;
            localRotateZ = rotateZ;
            localEndRender = endRender;
        }

        if (localEndRender)
        {
            break;
        }

        /*
         * Process one VR event.
         */
        interactor->DoOneEvent(window, renderer);

        /*
         * Apply animation roughly every 20 ms.
         */
        auto timeNow = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - t_last).count() > 20)
        {
            vtkActorCollection* actorList = renderer->GetActors();

            if (actorList != nullptr)
            {
                vtkActor* actor = nullptr;

                /*
                 * X rotation.
                 */
                actorList->InitTraversal();
                while ((actor = actorList->GetNextActor()) != nullptr)
                {
                    actor->RotateX(localRotateX);
                }

                /*
                 * Y rotation.
                 */
                actorList->InitTraversal();
                while ((actor = actorList->GetNextActor()) != nullptr)
                {
                    actor->RotateY(localRotateY);
                }

                /*
                 * Z rotation.
                 */
                actorList->InitTraversal();
                while ((actor = actorList->GetNextActor()) != nullptr)
                {
                    actor->RotateZ(localRotateZ);
                }
            }

            window->Render();

            t_last = timeNow;
        }
    }

    /*
     * Clean up the VR window when the loop finishes.
     */
    if (window != nullptr)
    {
        window->Finalize();
    }

    interactor = nullptr;
    window = nullptr;
    renderer = nullptr;
    camera = nullptr;
}