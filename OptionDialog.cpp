
#include "optiondialog.h"
#include "ui_optiondialog.h"
/**Constructor for OptionDialog
* Set ups the UI widget for changing the colour of an STL file.
*/
OptionDialog::OptionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionDialog)
{
    ui->setupUi(this);
}
/**Destructor for optionDiolog
* Deletes the ui.
*@param None
*@return None
*/
OptionDialog::~OptionDialog()
{
    delete ui;
}
/** function to load from the ModelPart 
* gets the colour values of RGB for the ModelPart.
* gets the value of visible, and it's name.
* @param None
* @return None
*/
void OptionDialog::loadFromModelPart(ModelPart* part) {
    ui->lineEdit->setText(part->data(0).toString());
    ui->checkBox->setChecked(part->visible());
    ui->spinBox->setValue(part->getColourR());
    ui->spinBox_2->setValue(part->getColourG());
    ui->spinBox_3->setValue(part->getColourB());
}

void OptionDialog::saveToModelPart(ModelPart* part) {
    part->set(0, ui->lineEdit->text());
    part->setVisible(ui->checkBox->isChecked());
    part->setColour(
        ui->spinBox->value(),
        ui->spinBox_2->value(),
        ui->spinBox_3->value()
    );
}
