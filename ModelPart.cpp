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


ModelPart::ModelPart(const QList<QVariant>& data, ModelPart* parent )
    : m_itemData(data), m_parentItem(parent) {

    /* You probably want to give the item a default colour */
    set( 2, 255 );
    set( 3, 255 );
    set( 4, 255 );
    isVisible = true;

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
}


ModelPart::~ModelPart() {
    qDeleteAll(m_childItems);
}


void ModelPart::appendChild( ModelPart* item ) {
    /* Add another model part as a child of this part
     * (it will appear as a sub-branch in the treeview)
     */
    item->m_parentItem = this;
    m_childItems.append(item);
}


ModelPart* ModelPart::child( int row ) {
    /* Return pointer to child item in row below this item.
     */
    if (row < 0 || row >= m_childItems.size())
        return nullptr;
    return m_childItems.at(row);
}

int ModelPart::childCount() const {
    /* Count number of child items
     */
    return m_childItems.count();
}


int ModelPart::columnCount() const {
    /* Count number of columns (properties) that this item has.
     */
    return m_itemData.count();
}

QVariant ModelPart::data(int column) const {
    /* Return the data associated with a column of this item 
     *  Note on the QVariant type - it is a generic placeholder type
     *  that can take on the type of most Qt classes. It allows each 
     *  column or property to store data of an arbitrary type.
     */
    if (column < 0 || column >= m_itemData.size())
        return QVariant();
    return m_itemData.at(column);
}


void ModelPart::set(int column, const QVariant &value) {
    /* Set the data associated with a column of this item 
     */
    if (column < 0 || column >= m_itemData.size())
        return;

    m_itemData.replace(column, value);
}


ModelPart* ModelPart::parentItem() {
    return m_parentItem;
}


int ModelPart::row() const {
    /* Return the row index of this item, relative to it's parent.
     */
    if (m_parentItem)
        return m_parentItem->m_childItems.indexOf(const_cast<ModelPart*>(this));
    return 0;
}

void ModelPart::setColour(const unsigned char R, const unsigned char G, const unsigned char B) {
    set(2, R);
    set(3, G);
    set(4, B);
    if (actor != nullptr) {
        actor->GetProperty()->SetColor(R/255.0, G/255.0, B/255.0);
    };
}

unsigned char ModelPart::getColourR() {
    return data(2).toInt();
}

unsigned char ModelPart::getColourG() {
    return data(3).toInt();
}

unsigned char ModelPart::getColourB() {
    return data(4).toInt();
}

void ModelPart::setVisible(bool isVisible) {
    this->isVisible = isVisible;

    // Update column 1 so the TreeView reflects the change
    set(1, isVisible ? "true" : "false");

    if (actor != nullptr) {
        actor->SetVisibility(isVisible);
    }
}

bool ModelPart::visible() {
    return isVisible;
}


void ModelPart::loadSTL(QString fileName) {
    file = vtkSmartPointer<vtkSTLReader>::New();
    file->SetFileName(fileName.toStdString().c_str());
    file->Update();

    mapper = vtkSmartPointer<vtkDataSetMapper>::New(); // was vtkPolyDataMapper
    mapper->SetInputConnection(file->GetOutputPort());

    actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);
}

vtkSmartPointer<vtkActor> ModelPart::getActor() {
   /* This is a placeholder function that you will need to modify if you want to use it */
   /* Needs to return a smart pointer to the vtkActor to allow
    * part to be rendered.
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

//vtkActor* ModelPart::getNewActor() {
    /* This is a placeholder function that you will need to modify if you want to use it
     * 
     * The default mapper/actor combination can only be used to render the part in 
     * the GUI, it CANNOT also be used to render the part in VR. This means you need
     * to create a second mapper/actor combination for use in VR - that is the role
     * of this function. */
     
     
     /* 1. Create new mapper */
     
     /* 2. Create new actor and link to mapper */
     
     /* 3. Link the vtkProperties of the original actor to the new actor. This means 
      *    if you change properties of the original part (colour, position, etc), the
      *    changes will be reflected in the GUI AND VR rendering.
      *    
      *    See the vtkActor documentation, particularly the GetProperty() and SetProperty()
      *    functions.
      */
    

    /* The new vtkActor pointer must be returned here */
//    return nullptr;
    
//}

