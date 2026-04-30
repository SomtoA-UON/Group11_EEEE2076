/**     @file ModelPart.h
  *
  *     EEEE2076 - Software Engineering & VR Project
  *
  *     Template for model parts that will be added as treeview items
  *
  *     P Evans 2022
  */

#ifndef VIEWER_MODELPART_H
#define VIEWER_MODELPART_H

#include <QString>
#include <QList>
#include <QVariant>

  /* VTK headers */
#include <vtkSmartPointer.h>
#include <vtkActor.h>
#include <vtkSTLReader.h>
#include <vtkColor.h>
#include <vtkPolyDataMapper.h>

class ModelPart
{
public:
    /** Constructor
     * @param data is a list of strings/properties for this item
     * @param parent is the parent of this item in the tree
     */
    ModelPart(const QList<QVariant>& data, ModelPart* parent = nullptr);

    /** Destructor
      * Frees all child items
      */
    ~ModelPart();

    /** Add a child to this item
      * @param item pointer to child object
      */
    void appendChild(ModelPart* item);

    /** Return child at position row below this item
      * @param row child row number
      * @return pointer to child item
      */
    ModelPart* child(int row);

    /** Return number of children
      * @return number of children
      */
    int childCount() const;

    /** Return number of columns/properties
      * @return number of visible data columns
      */
    int columnCount() const;

    /** Return the data item at a particular column
      * @param column column index
      * @return QVariant data value
      */
    QVariant data(int column) const;

    /** Set a property value
      * @param column column index
      * @param value value to set
      */
    void set(int column, const QVariant& value);

    /** Get pointer to parent item
      * @return pointer to parent item
      */
    ModelPart* parentItem();

    /** Get row index of this item relative to parent
      * @return row index
      */
    int row() const;

    /** Set colour using 0-255 RGB values
      * @param R red value
      * @param G green value
      * @param B blue value
      */
    void setColour(const unsigned char R,
        const unsigned char G,
        const unsigned char B);

    /** Get red colour value
      * @return red value, 0-255
      */
    unsigned char getColourR();

    /** Get green colour value
      * @return green value, 0-255
      */
    unsigned char getColourG();

    /** Get blue colour value
      * @return blue value, 0-255
      */
    unsigned char getColourB();

    /** Set visible flag
      * @param isVisible true if visible, false if hidden
      */
    void setVisible(bool isVisible);

    /** Get visible flag
      * @return true if visible
      */
    bool visible();

    /** Load STL file
      * @param fileName STL file path
      */
    void loadSTL(QString fileName);

    /** Return GUI actor
      * @return pointer to actor used in the normal Qt/VTK renderer
      */
    vtkSmartPointer<vtkActor> getActor();

    /** Return a new actor for VR
      *
      * Important:
      * This does NOT return the normal GUI actor.
      * It creates a new mapper and a new actor for the VR renderer.
      *
      * @return pointer to new actor used in VR
      */
    vtkSmartPointer<vtkActor> getNewActor();

private:
    QList<ModelPart*>                       m_childItems;   /**< Child items */
    QList<QVariant>                         m_itemData;     /**< Column data */
    ModelPart* m_parentItem;   /**< Parent item */

    bool                                    isVisible;      /**< Visibility flag */

    vtkSmartPointer<vtkSTLReader>           file;           /**< STL reader */
    vtkSmartPointer<vtkPolyDataMapper>      mapper;         /**< GUI mapper */
    vtkSmartPointer<vtkActor>               actor;          /**< GUI actor */

    vtkColor3<unsigned char>                colour;         /**< RGB colour */
};

#endif