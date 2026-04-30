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
#include <vtkSmartPointer.h>
#include <vtkSTLReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkPlane.h>
#include <vtkClipDataSet.h>
#include <vtkShrinkFilter.h>

/* VTK headers - will be needed when VTK used in next worksheet,
 * commented out for now
 *
 * Note that there are a few function definitions and variables
 * commented out below - this is because you haven't yet installed
 * the VTK library which is needed.
 */
#include <vtkSmartPointer.h>
#include <vtkMapper.h>
#include <vtkActor.h>
#include <vtkSTLReader.h>
#include <vtkColor.h>
#include <vtkSTLReader.h>
#include <vtkPolyDataMapper.h>
#include <vtkActor.h>
#include <vtkSmartPointer.h>

class ModelPart {
public:
    /** Constructor
     * @param data is a List (array) of strings for each property of this item (part name and visiblity in our case
     * @param parent is the parent of this item (one level up in tree)
     */
    ModelPart(const QList<QVariant>& data, ModelPart* parent = nullptr);

    /** Destructor
      * Needs to free array of child items
      */
    ~ModelPart();

    /** Add a child to this item.
      * @param item Pointer to child object (must already be allocated using new)
      */
    void appendChild(ModelPart* item);

    /** Return child at position 'row' below this item
      * @param row is the row number (below this item)
      * @return pointer to the item requested.
      */
    ModelPart* child(int row);

    /** Return number of children to this item
      * @return number of children
      */
    int childCount() const;         /* Note on the 'const' keyword - it means that this function is valid for
                                     * constant instances of this class. If a class is declared 'const' then it
                                     * cannot be modifed, this means that 'set' type functions are usually not
                                     * valid, but 'get' type functions are.
                                     */

    /** Get number of data items (2 - part name and visibility string) in this case.
      * @return number of visible data columns
      */
    int columnCount() const;

    /** Return the data item at a particular column for this item.
      * i.e. either part name of visibility
      * used by Qt when displaying tree
      * @param column is column index
      * @return the QVariant (represents string)
      */
    QVariant data(int column) const;


    /** Default function required by Qt to allow setting of part
      * properties within treeview.
      * @param column is the index of the property to set
      * @param value is the value to apply
      */
    void set( int column, const QVariant& value );

    /** Get pointer to parent item
      * @return pointer to parent item
      */
    ModelPart* parentItem();

    /** Get row index of item, relative to parent item
      * @return row index
      */
    int row() const;


    /** Set colour
      * (0-255 RGB values as ints)
      */
    void setColour(const unsigned char R, const unsigned char G, const unsigned char B);

    unsigned char getColourR();
    unsigned char getColourG();
    unsigned char getColourB();

    /** Set visible flag
      * @param isVisible sets visible/non-visible
      */
    void setVisible(bool isVisible);

    /** Get visible flag
      * @return visible flag as boolean 
      */
    bool visible();
	
	/** Load STL file
      * @param fileName
      */
    void loadSTL(QString fileName);

    /** Return actor
      * @return pointer to default actor for GUI rendering
      */
    vtkSmartPointer<vtkActor> getActor();

    /** Return new actor for use in VR
      * @return pointer to new actor
      */
    //vtkActor* getNewActor();

    bool getClipEnabled() const { return m_clipEnabled; }
    bool getShrinkEnabled() const { return m_shrinkEnabled; }
    void setClipEnabled(bool enabled) { m_clipEnabled = enabled; }
    void setShrinkEnabled(bool enabled) { m_shrinkEnabled = enabled; }
    void updatePipeline();

    // Getters/setters
    float getShrinkFactor() const { return m_shrinkFactor; }
    int   getClipOrigin() const { return m_clipOrigin; }
    void  setShrinkFactor(float f) { m_shrinkFactor = f; }
    void  setClipOrigin(int origin) { m_clipOrigin = origin; }

private:
    QList<ModelPart*>                           m_childItems;
    QList<QVariant>                             m_itemData;
    ModelPart* m_parentItem;

    bool                                        isVisible;

    vtkSmartPointer<vtkSTLReader>               file;
    vtkSmartPointer<vtkMapper>                  mapper;
    vtkSmartPointer<vtkActor>                   actor;
    vtkColor3<unsigned char>                    colour;

    bool m_clipEnabled;
    bool m_shrinkEnabled;

    vtkSmartPointer<vtkSTLReader>               m_reader;
    vtkSmartPointer<vtkPolyDataMapper>          m_mapper;
    vtkSmartPointer<vtkActor>                   m_actor;
    vtkSmartPointer<vtkPlane>                   m_clipPlane;    // only once
    vtkSmartPointer<vtkClipDataSet>             m_clipFilter;
    vtkSmartPointer<vtkShrinkFilter>            m_shrinkFilter;
    // Private members
    float m_shrinkFactor;
    int   m_clipOrigin;
};



#endif