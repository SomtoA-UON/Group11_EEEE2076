
/**@file OptionDialog.cpp 
* This file contains the code relating to the widget for changing the colour of an STL file.
*/
#include "optiondialog.h"
#include "ui_optiondialog.h"
#include <QPalette>
#include <QColor>

/**Constructor for OptionDialog
* Set ups the UI widget for changing the colour of an STL file.
*/
OptionDialog::OptionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionDialog)
{
    ui->setupUi(this);

    // Enhanced color scheme: deep blue background, accent colors, rounded corners, and subtle shadows
    QPalette palette;
    palette.setColor(QPalette::Window, QColor(28, 36, 58)); // deep blue background
    palette.setColor(QPalette::WindowText, QColor(240, 240, 255)); // light text
    palette.setColor(QPalette::Base, QColor(38, 48, 78)); // input fields
    palette.setColor(QPalette::AlternateBase, QColor(48, 58, 98));
    palette.setColor(QPalette::Text, QColor(240, 240, 255));
    palette.setColor(QPalette::Button, QColor(44, 62, 80)); // button background
    palette.setColor(QPalette::ButtonText, QColor(255, 255, 255));
    palette.setColor(QPalette::Highlight, QColor(0, 180, 216)); // cyan accent
    palette.setColor(QPalette::HighlightedText, QColor(0, 0, 0));
    this->setPalette(palette);
    this->setStyleSheet(
        "QDialog { "
        "  border-radius: 12px; "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #1c243a, stop:1 #304070); "
        "  border: 2px solid #00b4d8; "
        "  box-shadow: 0 8px 24px rgba(0,0,0,0.25); "
        "} "
        "QLabel { color: #f0f0ff; font-weight: bold; font-size: 13px; } "
        "QLineEdit, QSpinBox { "
        "  background: #26304e; color: #f0f0ff; border-radius: 6px; border: 1px solid #00b4d8; padding: 2px 8px; font-size: 13px; "
        "} "
        "QLineEdit:focus, QSpinBox:focus { border: 2px solid #90e0ef; } "
        "QCheckBox { color: #f0f0ff; font-size: 13px; } "
        "QCheckBox::indicator { border-radius: 4px; width: 18px; height: 18px; border: 1px solid #00b4d8; background: #26304e; } "
        "QCheckBox::indicator:checked { background: #00b4d8; border: 2px solid #90e0ef; } "
        "QPushButton, QDialogButtonBox QPushButton { "
        "  background: qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 #00b4d8, stop:1 #0077b6); "
        "  color: white; border-radius: 6px; padding: 6px 18px; font-size: 14px; font-weight: bold; border: none; "
        "} "
        "QPushButton:hover, QDialogButtonBox QPushButton:hover { background: #90e0ef; color: #222; } "
        "QDialogButtonBox { background: transparent; border: none; } "
    );
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
* @param ModelPart*part, the part that needs to be coloured
* @return None
*/
void OptionDialog::loadFromModelPart(ModelPart* part) {
    ui->lineEdit->setText(part->data(0).toString());
    ui->checkBox->setChecked(part->visible());
    ui->spinBox->setValue(part->getColourR());
    ui->spinBox_2->setValue(part->getColourG());
    ui->spinBox_3->setValue(part->getColourB());
}
/** Function for sacing the colour changes to the UI
* makes the colour changes set by the user to be visible on the UI
* @param ModelPart* part, the part which has been changed in colour.
* @return None
*/
void OptionDialog::saveToModelPart(ModelPart* part) {
    part->set(0, ui->lineEdit->text());
    part->setVisible(ui->checkBox->isChecked());
    part->setColour(
        ui->spinBox->value(),
        ui->spinBox_2->value(),
        ui->spinBox_3->value()
    );
}
