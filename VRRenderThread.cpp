void VRRenderThread::updateActor(vtkActor* oldActor, vtkActor* newActor) {
    QMutexLocker lock(&m_mutex);
    m_renderer->RemoveActor(oldActor);
    m_renderer->AddActor(newActor);
}