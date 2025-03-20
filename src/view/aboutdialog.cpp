#include "fge/view/aboutdialog.h"
#include "ui_aboutdialog.h"

#include <qnamespace.h>
#include <qsvgwidget.h>

const auto logoFile = QString(":resources/logo_fge.svg");


class LogoWidget: public QSvgWidget
{
public:
	LogoWidget( QWidget *parent = nullptr )
		: QSvgWidget( parent )
	{
		init();
	}
	LogoWidget( const QString &file, QWidget *parent = nullptr )
		: QSvgWidget( file, parent )
	{
		init();
	}
	virtual QSize sizeHint() const override {
		return QSize(300,200);
	};
private:
	void init() {
		QSizePolicy policy(
				QSizePolicy::Fixed,
				QSizePolicy::Fixed
		);
		setSizePolicy( policy );
	}
};


AboutDialog::AboutDialog(
		const QString& content,
		QWidget *parent
)
	: QDialog(parent)
		, ui(new Ui::AboutDialog)
{
	ui->setupUi(this);
	ui->textEdit->setText( content );
	auto logoWidget = new LogoWidget(
			logoFile
	);
	ui->verticalLayout_2->insertWidget(
			0,
			logoWidget,
			0,
			Qt::AlignCenter
	);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
