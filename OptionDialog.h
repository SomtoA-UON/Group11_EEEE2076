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
    explicit OptionDialog(QWidget *parent = nullptr);
    ~OptionDialog();

    void loadFromModelPart(ModelPart* part);
    void saveToModelPart(ModelPart* part);

private:
    Ui::OptionDialog *ui;    // <-- pointer to the generated UI class
};

#endif
