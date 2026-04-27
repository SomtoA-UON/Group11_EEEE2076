#include "optiondialog.h"
#include "ui_optiondialog.h"

OptionDialog::OptionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionDialog)
{
    ui->setupUi(this);
}

OptionDialog::~OptionDialog()
{
    delete ui;
}

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
