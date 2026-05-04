/**     @file VRRenderThread.cpp
* this file contains the code to render in VR.
  */

#include "VRRenderThread.h"

  /* Qt headers */
#include <QMutexLocker>

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
#include <vtkPlaneSource.h>
#include <vtkPolyDataMapper.h>

/** Constructor for VR
 * This is called by MainWindow in the main GUI thread.
 * The VR renderer itself starts when start() is called, it then sets the rotation values to 0
 * @param QObject* parent
 * @return None
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
 * Destructor for VR
 * Stops the VR loop safely if it is still running.
 * @param None
 * @return None
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
 * Add actor before VR starts.
 * Do not pass the normal GUI actor here.
 * MainWindow should pass a new VR actor created by ModelPart::getNewActor().
 * @param VtkActor* actor.
 * @return None
 */
void VRRenderThread::addActorOffline(vtkActor* actor)
{
    if (actor == nullptr)
    {
        return;
    }

    /*
     * Only allow actors to be added before the thread is running.
     * This avoids editing VTK objects from the GUI thread while VR is active.
     */
    if (!isRunning())
    {
        QMutexLocker locker(&mutex);

        /*
         * IMPORTANT:
         * Do not rotate or move the actor here.
         *
         * Some students had an issue where only a white cube appeared in VR.
         * The fix is to remove the RotateX/AddPosition code from this function
         * and apply the positioning later in run(), after actors are added to
         * the VR renderer.
         */
        actors->AddItem(actor);
    }
}

/** Send command function.
 * Send command to the VR thread.
 * @param integer named cmd, and a double which is named value.
 * @return None
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
/**Update actor function
* Updates the actor, by removing the old actor, and replacing it with the new
* @param vtkActor* oldActor and vtkActor*newActor
* @return None.
*/
void VRRenderThread::updateActor(vtkActor* oldActor, vtkActor* newActor) {
    QMutexLocker lock(&mutex);
    renderer->RemoveActor(oldActor);
    renderer->AddActor(newActor);
}

/** Main VR render thread.
*This runs separately from the Qt GUI thread.
* @param None
* @return None
*/
void VRRenderThread::run()
{
    /*
     * Create background colour.
     */
    vtkNew<vtkNamedColors> colors;

    std::array<unsigned char, 4> backgroundColour{ {26, 51, 102, 255} };
    colors->SetColor("BkgColor", backgroundColour.data());

    /*
     * Create OpenVR renderer.
     */
    renderer = vtkSmartPointer<vtkOpenVRRenderer>::New();
    renderer->SetBackground(colors->GetColor3d("BkgColor").GetData());

    /*
     * Add all offline actors to the VR renderer.
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
     * Create OpenVR render window.
     */
    window = vtkSmartPointer<vtkOpenVRRenderWindow>::New();

    window->Initialize();
    window->AddRenderer(renderer);

    /*
     * Create OpenVR camera.
     */
    camera = vtkSmartPointer<vtkOpenVRCamera>::New();
    renderer->SetActiveCamera(camera);

    /*
     * Add simple light to scene.
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
     * Create OpenVR interactor.
     */
    interactor = vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New();

    interactor->SetRenderWindow(window);
    interactor->Initialize();

    /*
     * Reintroduce initial model positioning here instead of in addActorOffline().
     *
     * This is the fix your lecturer mentioned:
     * - add actors first
     * - then rotate/move them after they are inside the VR renderer
     */
    vtkActorCollection* actorListForInitialTransform = renderer->GetActors();

    if (actorListForInitialTransform != nullptr)
    {
        vtkActor* a = nullptr;

        actorListForInitialTransform->InitTraversal();

        while ((a = actorListForInitialTransform->GetNextActor()) != nullptr)
        {
            double* ac = a->GetOrigin();

            a->RotateX(-90.0);

            a->AddPosition(
                -ac[0] + 0.0,
                -ac[1] - 100.0,
                -ac[2] - 200.0
            );
        }
    }

    // --- Create a floor plane ---

    vtkNew<vtkPlaneSource> floorSource;
    floorSource->SetOrigin(-500.0, 0.0, -500.0);
    floorSource->SetPoint1(500.0, 0.0, -500.0);
    floorSource->SetPoint2(-500.0, 0.0, 500.0);
    floorSource->Update();

    vtkNew<vtkPolyDataMapper> floorMapper;
    floorMapper->SetInputConnection(floorSource->GetOutputPort());

    vtkNew<vtkActor> floorActor;
    floorActor->SetMapper(floorMapper);
    floorActor->GetProperty()->SetColor(0.3, 0.3, 0.3);

    // optional: make it slightly transparent
    floorActor->GetProperty()->SetOpacity(0.8);

    renderer->AddActor(floorActor);

    renderer->ResetCamera();
    window->Render();

    /*
     * Start manual VR event loop.
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
         * Copy command variables safely.
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
         * Apply animation every 20 ms.
         */
        auto timeNow = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(timeNow - t_last).count() > 20)
        {
            vtkActorCollection* actorList = renderer->GetActors();

            if (actorList != nullptr)
            {
                vtkActor* actor = nullptr;

                actorList->InitTraversal();

                while ((actor = actorList->GetNextActor()) != nullptr)
                {
                    actor->RotateX(localRotateX);
                    actor->RotateY(localRotateY);
                    actor->RotateZ(localRotateZ);
                }
            }

            window->Render();

            t_last = timeNow;
        }
    }

    /*
     * Clean shutdown.
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