#include "fge/view/helpdialog.h"
#include "ui_helpdialog.h"


const auto helpString = QStringList{
		"# Key Codes",
		"## Graph View",
		"",
		"Click on a *Graph View* to select it. Now, you may use the following controls to change the view properties:",
		"",
		"### Reset View",
		"",
		"Function|Keys|Alt. Keys",
		"-|-|-",
		"Reset View|Strg+0|",
		"",
		"### Scale / Zoom",
		"",
		"Function|Keys|Alt. Keys",
		"-|-|-",
		"Zoom|Strg+Mousewheel|Strg+Plus / Strg+Minus",
		"Zoom horizontally|Strg+Alt+Mousewheel|Strg+Alt+Left / Strg+Alt+Right",
		"Zoom vertically|Strg+Shift+Mousewheel|Strg+Shift+Left / Strg+Shift+Right",
		"",
		"### Translate / Move",
		"",
		"Function|Keys|Alt. Keys",
		"-|-|-",
		"Translate horizontally|Alt+Mousewheel|Left / Right",
		"Translate vertically|(Shift+)Mousewheel|Up / Down"
}.join("\n");

HelpDialog::HelpDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::HelpDialog)
{
    ui->setupUi(this);
		ui->textEdit->setMarkdown( helpString );
}

HelpDialog::~HelpDialog()
{
    delete ui;
}
