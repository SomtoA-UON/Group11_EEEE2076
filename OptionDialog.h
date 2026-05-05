/** @file OptionDialog.h
* This file is used as the header file for the widget relating to adjusting the colour of the 3D model
*/
#ifndef OPTIONDIALOG_H
#define OPTIONDIALOG_H

#include <QDialog>
#include "ModelPart.h"

namespace Ui {
class OptionDialog;      // <-- forward declaration, must match the class name in your .ui file
}

class OptionDialog : public QDialog
{
    Q_OBJECT

public:
    /**Constructor for optionDialog widget.
    * @param QWidget *parent, parent widget, which is in mainwindow.
    * @return None.
    */
    explicit OptionDialog(QWidget *parent = nullptr);
    /**destructor for optionDialog widget.
    * @param None
    * @return None
    */
    ~OptionDialog();
    /**function to load the saved colour settings already applied.
    * @param ModelPart* part
    * @return None
    */
    void loadFromModelPart(ModelPart* part);
    /**function to save the colour settings, the user has changed.
    * @param ModelPart* part
    * @return None
    */
    void saveToModelPart(ModelPart* part);

private:
    Ui::OptionDialog *ui;    // <-- pointer to the generated UI class
};

#endif
