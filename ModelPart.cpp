/**     @file ModelPart.cpp
  *
  *     EEEE2076 - Software Engineering & VR Project
  *
  *     Template for model parts that will be added as treeview items
  *
  *     P Evans 2022
  */

#include "ModelPart.h"

#include <QDebug>

  /* Commented out for now, will be uncommented later when you have
   * installed the VTK library
   */
#include <vtkSmartPointer.h>
#include <vtkDataSetMapper.h>
#include <vtkProperty.h>
#include <vtkPlane.h>

/**
 * Constructor.
 */
ModelPart::ModelPart(const QList<QVariant>& data, ModelPart* parent)
    : m_itemData(data),
    m_parentItem(parent)
{
    /*
     * Your tree uses five columns:
     * 0 = part name
     * 1 = visible
     * 2 = red
     * 3 = green
     * 4 = blue
     *
     * This makes sure the list always has enough columns.
     */
    while (m_itemData.size() < 5)
    {
        m_itemData.append(QVariant());
    }

ModelPart::ModelPart(const QList<QVariant>& data, ModelPart* parent)
    : m_itemData(data), m_parentItem(parent) {

    // Defaults for Filter Portion
    m_clipPlane = vtkSmartPointer<vtkPlane>::New();
    m_clipPlane->SetOrigin(0.0, 0.0, 0.0);
    m_clipPlane->SetNormal(-1.0, 0.0, 0.0); // clips along X axis

    m_clipEnabled = false;
    m_shrinkEnabled = false;

    m_reader = vtkSmartPointer<vtkSTLReader>::New();
    m_mapper = vtkSmartPointer<vtkPolyDataMapper>::New();
    m_actor = vtkSmartPointer<vtkActor>::New();
    m_clipFilter = vtkSmartPointer<vtkClipDataSet>::New();
    m_shrinkFilter = vtkSmartPointer<vtkShrinkFilter>::New();

    m_mapper->SetInputConnection(m_reader->GetOutputPort());
    m_actor->SetMapper(m_mapper);

    m_shrinkFactor = 0.8f;
    m_clipOrigin = 0;

    set(1, "true");

    setColour(255, 255, 255);
}

/**
 * Destructor.
 */
ModelPart::~ModelPart()
{
    qDeleteAll(m_childItems);
}

/**
 * Add child item.
 */
void ModelPart::appendChild(ModelPart* item)
{
    if (item == nullptr)
    {
        return;
    }

void ModelPart::appendChild(ModelPart* item) {
    item->m_parentItem = this;
    m_childItems.append(item);
}

void ModelPart::removeChild(int row) {
    if (row < 0 || row >= m_childItems.size())
        return;
    ModelPart* child = m_childItems.takeAt(row);
    delete child;   /* destructor recursively deletes grandchildren */
}


ModelPart* ModelPart::child(int row) {
    /* Return pointer to child item in row below this item.
     */
    if (row < 0 || row >= m_childItems.size())
    {
        return nullptr;
    }

    return m_childItems.at(row);
}

/**
 * Return number of children.
 */
int ModelPart::childCount() const
{
    return m_childItems.count();
}

/**
 * Return number of columns.
 */
int ModelPart::columnCount() const
{
    return m_itemData.count();
}

QVariant ModelPart::data(int column) const {
    /* Return the data associated with a column of this item
     *  Note on the QVariant type - it is a generic placeholder type
     *  that can take on the type of most Qt classes. It allows each
     *  column or property to store data of an arbitrary type.
     */
    if (column < 0 || column >= m_itemData.size())
    {
        return QVariant();
    }

    return m_itemData.at(column);
}

void ModelPart::set(int column, const QVariant& value) {
    /* Set the data associated with a column of this item
     */
    if (column < 0 || column >= m_itemData.size())
    {
        return;
    }

    m_itemData.replace(column, value);
}

/**
 * Return parent item.
 */
ModelPart* ModelPart::parentItem()
{
    return m_parentItem;
}

/**
 * Return row index.
 */
int ModelPart::row() const
{
    if (m_parentItem)
    {
        return m_parentItem->m_childItems.indexOf(
            const_cast<ModelPart*>(this)
        );
    }

    return 0;
}

/**
 * Set colour.
 */
void ModelPart::setColour(const unsigned char R,
    const unsigned char G,
    const unsigned char B)
{
    colour.SetRed(R);
    colour.SetGreen(G);
    colour.SetBlue(B);

    set(2, static_cast<int>(R));
    set(3, static_cast<int>(G));
    set(4, static_cast<int>(B));

    /*
     * VTK uses colour values between 0.0 and 1.0,
     * but the GUI/tree stores them between 0 and 255.
     */
    if (actor != nullptr)
    {
        actor->GetProperty()->SetColor(
            static_cast<double>(R) / 255.0,
            static_cast<double>(G) / 255.0,
            static_cast<double>(B) / 255.0
        );
    }
}

/**
 * Get red value.
 */
unsigned char ModelPart::getColourR()
{
    return static_cast<unsigned char>(data(2).toInt());
}

/**
 * Get green value.
 */
unsigned char ModelPart::getColourG()
{
    return static_cast<unsigned char>(data(3).toInt());
}

/**
 * Get blue value.
 */
unsigned char ModelPart::getColourB()
{
    return static_cast<unsigned char>(data(4).toInt());
}

/**
 * Set visibility.
 */
void ModelPart::setVisible(bool isVisible)
{
    this->isVisible = isVisible;

    set(1, isVisible ? "true" : "false");

    if (actor != nullptr)
    {
        actor->SetVisibility(isVisible);
    }
}

/**
 * Return visibility.
 */
bool ModelPart::visible()
{
    return isVisible;
}

void ModelPart::loadSTL(QString fileName)
{
    /*
     * 1. Create STL reader.
     */
    file = vtkSmartPointer<vtkSTLReader>::New();
    file->SetFileName(fileName.toStdString().c_str());
    file->Update();

    mapper = vtkSmartPointer<vtkDataSetMapper>::New(); // was vtkPolyDataMapper
    mapper->SetInputConnection(file->GetOutputPort());

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);


    /*
     * 4. Apply current colour and visibility.
     */
    actor->GetProperty()->SetColor(
        static_cast<double>(getColourR()) / 255.0,
        static_cast<double>(getColourG()) / 255.0,
        static_cast<double>(getColourB()) / 255.0
    );

    actor->SetVisibility(isVisible);

    qDebug() << "STL loaded:" << fileName;
    qDebug() << "Actor is null?" << (actor == nullptr);
}

vtkSmartPointer<vtkActor> ModelPart::getActor() {
    /* Return GUI
     */

    return actor;
}

void ModelPart::updatePipeline() {
    vtkAlgorithmOutput* output = file->GetOutputPort();

    if (m_clipEnabled) {
        m_clipPlane->SetOrigin(m_clipOrigin, 0.0, 0.0); // slider moves clip along X
        m_clipFilter->SetInputConnection(output);
        m_clipFilter->SetClipFunction(m_clipPlane.Get());
        output = m_clipFilter->GetOutputPort();
    }

    if (m_shrinkEnabled) {
        m_shrinkFilter->SetInputConnection(output);
        m_shrinkFilter->SetShrinkFactor(m_shrinkFactor);
        m_shrinkFilter->Update();
        output = m_shrinkFilter->GetOutputPort();
    }

    mapper->SetInputConnection(output);
}

/**
 * Create and return a new actor for VR.
 *
 * This is important:
 *
 * The GUI renderer already uses:
 * STL reader -> GUI mapper -> GUI actor
 *
 * The VR renderer must use:
 * same STL reader -> new VR mapper -> new VR actor
 *
 * Do not return the normal GUI actor here.
 */
vtkSmartPointer<vtkActor> ModelPart::getNewActor()
{
    if (file == nullptr || actor == nullptr)
    {
        return nullptr;
    }

    /*
     * 1. Create a new mapper for VR.
     * This mapper uses the same STL reader output as the GUI mapper.
     */
    vtkSmartPointer<vtkPolyDataMapper> newMapper =
        vtkSmartPointer<vtkPolyDataMapper>::New();

    newMapper->SetInputConnection(file->GetOutputPort());

    /*
     * 2. Create a new actor for VR.
     */
    vtkSmartPointer<vtkActor> newActor =
        vtkSmartPointer<vtkActor>::New();

    newActor->SetMapper(newMapper);

    /*
     * 3. Link the property from the GUI actor.
     *
     * This means colour/material changes on the GUI actor can also be seen
     * by the VR actor because they share the same vtkProperty object.
     */
    newActor->SetProperty(actor->GetProperty());

    /*
     * 4. Copy visibility and transform values.
     */
    newActor->SetVisibility(actor->GetVisibility());

    double position[3];
    actor->GetPosition(position);
    newActor->SetPosition(position);

    double orientation[3];
    actor->GetOrientation(orientation);
    newActor->SetOrientation(orientation);

    double scale[3];
    actor->GetScale(scale);
    newActor->SetScale(scale);

    double origin[3];
    actor->GetOrigin(origin);
    newActor->SetOrigin(origin);

    return newActor;
}
