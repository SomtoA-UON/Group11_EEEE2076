/**     @file VRRenderThread.cpp
* this file contains the code to render in VR.
  */

#include "VRRenderThread.h"

  /* Qt headers */
#include <QMutexLocker>
#include <QDebug>
#include <QFile>
#include <QCoreApplication>

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
#include <vtkSkybox.h>
#include <vtkTexture.h>
#include <vtkJPEGReader.h>

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

    resetView = false;
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

    if (!isRunning())
    {
        QMutexLocker locker(&mutex);
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

    case RESET_VIEW:
        resetView = true;
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
    /* ------------------------------------------------------------------ */
    /* 1. Create renderer                                                  */
    /* ------------------------------------------------------------------ */
    renderer = vtkSmartPointer<vtkOpenVRRenderer>::New();
    renderer->SetBackground(0.1, 0.2, 0.4);   /* fallback dark blue */

    /* ------------------------------------------------------------------ */
    /* 2. Add model actors                                                 */
    /* ------------------------------------------------------------------ */
    {
        QMutexLocker locker(&mutex);

        vtkActor* actor = nullptr;
        actors->InitTraversal();

        while ((actor = actors->GetNextActor()) != nullptr)
        {
            renderer->AddActor(actor);
        }
    }

    /* ------------------------------------------------------------------ */
    /* 3. Create window and attach renderer                                */
    /* ------------------------------------------------------------------ */
    window = vtkSmartPointer<vtkOpenVRRenderWindow>::New();
    window->Initialize();
    window->AddRenderer(renderer);

    /* ------------------------------------------------------------------ */
    /* 4. Camera                                                           */
    /* ------------------------------------------------------------------ */
    camera = vtkSmartPointer<vtkOpenVRCamera>::New();
    renderer->SetActiveCamera(camera);

    /* ------------------------------------------------------------------ */
    /* 5. Lighting                                                         */
    /* ------------------------------------------------------------------ */
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

    /* ------------------------------------------------------------------ */
    /* 6. Interactor                                                       */
    /* ------------------------------------------------------------------ */
    interactor = vtkSmartPointer<vtkOpenVRRenderWindowInteractor>::New();
    interactor->SetRenderWindow(window);
    interactor->Initialize();

    /* ------------------------------------------------------------------ */
    /* 7. Initial model positioning                                        */
    /*    Done here AFTER actors are in the renderer, NOT in               */
    /*    addActorOffline() - this avoids the white cube bug               */
    /* ------------------------------------------------------------------ */
    {
        vtkActorCollection* actorList = renderer->GetActors();

        if (actorList != nullptr)
        {
            vtkActor* a = nullptr;
            actorList->InitTraversal();

            while ((a = actorList->GetNextActor()) != nullptr)
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
        /* Save original transforms for reset */
        {
            vtkActorCollection* actorList = renderer->GetActors();
            if (actorList != nullptr)
            {
                vtkActor* a = nullptr;
                actorList->InitTraversal();
                while ((a = actorList->GetNextActor()) != nullptr)
                {
                    ActorTransform t;
                    a->GetPosition(t.position);
                    a->GetOrientation(t.orientation);
                    a->GetScale(t.scale);
                    a->GetOrigin(t.origin);
                    originalTransforms.append(t);
                }
            }
        }
    }

    /* ------------------------------------------------------------------ */
    /* 8. Floor plane                                                      */
    /* ------------------------------------------------------------------ */
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
    floorActor->GetProperty()->SetOpacity(0.8);
    renderer->AddActor(floorActor);

    /* ------------------------------------------------------------------ */
    /* 9. Skybox                                                           */
    /*    Added LAST so it is never affected by the model transform loop   */
    /* ------------------------------------------------------------------ */
    QString appDir = QCoreApplication::applicationDirPath();
    QString skyboxDir = appDir + "/skybox/";

    QStringList faceFiles = {
        skyboxDir + "right.jpg",
        skyboxDir + "left.jpg",
        skyboxDir + "top.jpg",
        skyboxDir + "bottom.jpg",
        skyboxDir + "front.jpg",
        skyboxDir + "back.jpg"
    };

    qDebug() << "Looking for skybox in:" << skyboxDir;

    bool skyboxValid = true;
    for (const QString& f : faceFiles)
    {
        if (!QFile::exists(f))
        {
            qDebug() << "Skybox image missing:" << f;
            skyboxValid = false;
            break;
        }
    }

    if (skyboxValid)
    {
        vtkNew<vtkTexture> skyboxTexture;
        skyboxTexture->CubeMapOn();
        skyboxTexture->InterpolateOn();
        skyboxTexture->RepeatOff();
        skyboxTexture->EdgeClampOn();
        skyboxTexture->MipmapOn();

        for (int i = 0; i < 6; i++)
        {
            vtkNew<vtkJPEGReader> reader;
            reader->SetFileName(faceFiles[i].toStdString().c_str());
            reader->Update();
            skyboxTexture->SetInputConnection(i, reader->GetOutputPort());
        }

        vtkNew<vtkSkybox> skybox;
        skybox->SetTexture(skyboxTexture);
        renderer->AddActor(skybox);

        qDebug() << "Skybox loaded successfully.";
    }
    else
    {
        /* Fallback: gradient background so it still looks decent */
        renderer->GradientBackgroundOn();
        renderer->SetBackground(0.53, 0.81, 0.98);   /* sky blue top */
        renderer->SetBackground2(0.36, 0.25, 0.20);  /* warm ground bottom */
        qDebug() << "Skybox not found - using gradient background.";
    }

    /* ------------------------------------------------------------------ */
    /* 10. First render                                                    */
    /* ------------------------------------------------------------------ */
    renderer->ResetCamera();
    window->Render();

    /* ------------------------------------------------------------------ */
    /* 11. Main VR event loop                                              */
    /* ------------------------------------------------------------------ */
    {

        /* Handle reset view */
        bool localResetView = false;
        {
            QMutexLocker locker(&mutex);
            localResetView = resetView;
            if (resetView) resetView = false;
        }

        if (localResetView)
        {
            vtkActorCollection* actorList = renderer->GetActors();
            if (actorList != nullptr)
            {
                vtkActor* a = nullptr;
                int i = 0;
                actorList->InitTraversal();
                while ((a = actorList->GetNextActor()) != nullptr && i < originalTransforms.size())
                {
                    a->SetPosition(originalTransforms[i].position);
                    a->SetOrientation(originalTransforms[i].orientation);
                    a->SetScale(originalTransforms[i].scale);
                    a->SetOrigin(originalTransforms[i].origin);
                    i++;
                }
            }
        }
        QMutexLocker locker(&mutex);
        endRender = false;
    }

    t_last = std::chrono::steady_clock::now();

    while (!interactor->GetDone())
    {
        double localRotateX = 0.0;
        double localRotateY = 0.0;
        double localRotateZ = 0.0;
        bool   localEndRender = false;

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

        interactor->DoOneEvent(window, renderer);

        auto timeNow = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(
            timeNow - t_last).count() > 20)
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

    /* ------------------------------------------------------------------ */
    /* 12. Clean shutdown                                                  */
    /* ------------------------------------------------------------------ */
    if (window != nullptr)
    {
        window->Finalize();
    }

    interactor = nullptr;
    window = nullptr;
    renderer = nullptr;
    camera = nullptr;
}