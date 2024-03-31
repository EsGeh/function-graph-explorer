#include "functionview.h"
#include "ui_functionview.h"

FunctionView::FunctionView(
		const QString& title,
		QWidget *parent
)
    : QWidget(parent)
    , ui(new Ui::FunctionView)
		, graphView(nullptr)
		, displayDialog(nullptr)
		, statusBar(nullptr)
{
	ui->setupUi(this);

	ui->formulaLabel->setText( title );
	statusBar = new QStatusBar();
	statusBar->setVisible(false);
	ui->verticalLayout->addWidget( statusBar, 0 );
	graphView = new GraphView();
	ui->verticalLayout->addWidget( graphView, 1 );

	displayDialog = new FunctionDisplayOptions(this);

	connect(
		ui->formulaEdit,
		&QLineEdit::textChanged,
		[this](QString value) {
			displayDialog->setFormula( value );
			emit formulaChanged();
		}
	);
	connect(
		ui->optionsBtn,
		&QAbstractButton::clicked,
		[this]() {
			displayDialog->show();
		}
	);
	connect(
		ui->playBtn,
		&QAbstractButton::clicked,
		[this]() {
			emit playButtonPressed(
					displayDialog->getPlaybackDuration(),
					displayDialog->getPlaybackSpeed(),
					displayDialog->getPlaybackOffset()
			);
		}
	);
	connect(
		displayDialog,
		&FunctionDisplayOptions::finished,
		[this](int result) {
			// qDebug() << "dialog closed with " << result;
			if( !result ) return;
			ui->formulaEdit->setText( displayDialog->getFormula() );
			graphView->setOrigin(
					displayDialog->getOrigin()
			);
			graphView->setScale(
					displayDialog->getScale()
			);
			emit formulaChanged();
		}
	);
	connect(
		this->graphView,
		&GraphView::viewChanged,
		[this]() {
			displayDialog->setOrigin(
					graphView->getOrigin()
			);
			displayDialog->setScale(
					graphView->getScale()
			);
			emit viewParamsChanged();
		}
	);
}

FunctionView::~FunctionView()
{
    delete ui;
}

QString FunctionView::getFormula() {
	return ui->formulaEdit->text();
}
GraphView* FunctionView::getGraphView() {
	return graphView;
}

void FunctionView::setFormulaError( const QString& str )
{
	statusBar->setVisible(true);
	statusBar->showMessage(str);
	graphView->reset();
}

void FunctionView::setFormula( const QString& str ) {
	ui->formulaEdit->setText( str );
}

void FunctionView::setGraph(
    const std::vector<std::pair<C,C>>& values
)
{
	statusBar->setVisible(false);
	graphView->setGraph( values );
}
