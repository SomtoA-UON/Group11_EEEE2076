/**
 * @file VRRenderThread.h
 *
 * EEEE2076 - Software Development Group Design Project
 *
 * VR rendering thread for the Qt/VTK application.
 */

#ifndef VR_RENDER_THREAD_H
#define VR_RENDER_THREAD_H

 /* Qt headers */
#include <QThread>
#include <QMutex>
#include <QWaitCondition>

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
 * This class runs the OpenVR renderer in a separate thread from the main Qt GUI.
 * Actors should be added before the thread is started using addActorOffline().
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
        ROTATE_Z
    };

    /**
     * Constructor.
     *
     * @param parent Parent QObject.
     */
    explicit VRRenderThread(QObject* parent = nullptr);

    /**
     * Destructor.
     */
    ~VRRenderThread() override;

    /**
     * Adds an actor to the VR scene before the VR thread has started.
     *
     * Important: this actor should NOT be the same actor used in the normal Qt renderer.
     * It should be a new actor with a new mapper.
     *
     * @param actor Actor to add to the VR renderer.
     */
    void addActorOffline(vtkActor* actor);

    /**
     * Sends a command to the VR thread in a thread-safe way.
     *
     * @param cmd Command from the Command enum.
     * @param value Value linked to the command.
     */
    void issueCommand(int cmd, double value);

protected:
    /**
     * Main VR thread function.
     *
     * This runs when VRRenderThread::start() is called.
     */
    void run() override;

private:
    /* Standard VTK VR objects */
    vtkSmartPointer<vtkOpenVRRenderWindow>              window;
    vtkSmartPointer<vtkOpenVRRenderWindowInteractor>    interactor;
    vtkSmartPointer<vtkOpenVRRenderer>                  renderer;
    vtkSmartPointer<vtkOpenVRCamera>                    camera;

    /* Used to synchronise commands between the GUI thread and VR thread */
    QMutex                                              mutex;
    QWaitCondition                                      condition;

    /* Actors that will be added to the VR scene */
    vtkSmartPointer<vtkActorCollection>                 actors;

    /* Timer used for animation updates */
    std::chrono::time_point<std::chrono::steady_clock>  t_last;

    /* If true, the VR render loop will end */
    bool                                                endRender;

    /* Rotation values applied during animation */
    double                                              rotateX;
    double                                              rotateY;
    double                                              rotateZ;
};

#endif