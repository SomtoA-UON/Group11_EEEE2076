/**     @file VRRenderThread.h
  *
  *     EEEE2076 - Software Engineering & VR Project
  *
  *     Template to add VR rendering to your application.
  */

#ifndef VR_RENDER_THREAD_H
#define VR_RENDER_THREAD_H

#include <vtkSkybox.h>
#include <vtkTexture.h>
#include <vtkJPEGReader.h>

  /* Qt headers */
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include <QList>

/* Standard headers */
#include <chrono>

/* VTK headers */
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkActorCollection.h>
#include <vtkOpenVRRenderWindow.h>
#include <vtkOpenVRRenderWindowInteractor.h>
#include <vtkOpenVRRenderer.h>
#include <vtkOpenVRCamera.h>

/**
 * @class VRRenderThread
 *
 * Runs the OpenVR renderer in a separate thread from the main Qt GUI.
 *
 * Actors should be added using addActorOffline() before the thread is started.
 */
class VRRenderThread : public QThread
{
    Q_OBJECT

public:
    /**
     * Commands that can be sent from the GUI thread to the VR thread.
     */
    enum Command
    {
        END_RENDER,
        ROTATE_X,
        ROTATE_Y,
        ROTATE_Z,
        RESET_VIEW   // <-- add this
    };

    /**
     * Constructor.
     */
    explicit VRRenderThread(QObject* parent = nullptr);

    /**
     * Destructor.
     */
    ~VRRenderThread() override;

    /**
     * Add an actor before the VR thread starts.
     *
     * Important:
     * This actor should be a separate VR actor, not the same actor used
     * in the normal Qt/VTK renderer.
     */
    void addActorOffline(vtkActor* actor);

    /**
     * Send a command to the VR thread.
     */
    void issueCommand(int cmd, double value);

    void updateActor(vtkActor* oldActor, vtkActor* newActor);

protected:
    /**
     * Main thread function. Runs when start() is called.
     */
    void run() override;

private:

    /* Stores original actor transforms for reset */
    struct ActorTransform {
        double position[3];
        double orientation[3];
        double scale[3];
        double origin[3];
    };
    QList<ActorTransform> originalTransforms;  /* add this */
    bool resetView;                             /* add this */
    /* Standard VTK VR classes */
    vtkSmartPointer<vtkOpenVRRenderWindow>              window;
    vtkSmartPointer<vtkOpenVRRenderWindowInteractor>    interactor;
    vtkSmartPointer<vtkOpenVRRenderer>                  renderer;
    vtkSmartPointer<vtkOpenVRCamera>                    camera;

    /* Used to synchronise commands from the GUI thread */
    QMutex                                              mutex;
    QWaitCondition                                      condition;

    /* Actors to add to the VR scene */
    vtkSmartPointer<vtkActorCollection>                 actors;

    /* Timer for animation */
    std::chrono::time_point<std::chrono::steady_clock>  t_last;

    /* Render loop control */
    bool                                                endRender;

    /* Animation rotation values */
    double                                              rotateX;
    double                                              rotateY;
    double                                              rotateZ;
};

#endif