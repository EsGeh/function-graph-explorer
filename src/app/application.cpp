#include "application.h"
#include <qnamespace.h>
#include <QPalette>


Application::Application(int& argc, char **argv)
		: QApplication(argc, argv)
{

	auto color1 = QColor("#008b8b");

	// modify palette to dark
	QPalette darkPalette;
	// Window (general background/foreground):
	darkPalette.setColor(
			QPalette::Window,
			QColor(53,53,53)
	);
	darkPalette.setColor(
			QPalette::WindowText,
			Qt::white
	);
	darkPalette.setColor(
			QPalette::Disabled,
			QPalette::WindowText,
			QColor(127,127,127)
	);
	// Base (background in text entry widgets and similar):
	darkPalette.setColor(
			QPalette::Base,
			QColor(42,42,42)
	);
	darkPalette.setColor(
			QPalette::AlternateBase,
			QColor(66,66,66)
	);
	// Text (foreground used with QPalette::Base):
	darkPalette.setColor(
			QPalette::Text,
			Qt::white
	);
	// Tooltip:
	darkPalette.setColor(
			QPalette::ToolTipBase,
			color1
	);
	darkPalette.setColor(
			QPalette::ToolTipText,
			Qt::white
	);
	// Placeholder Text:
	darkPalette.setColor(
			QPalette::PlaceholderText,
			Qt::white
	);
	// Buttons (background/foreground):
	darkPalette.setColor(
			QPalette::Button,
			QColor(53,53,53)
	);
	darkPalette.setColor(
			QPalette::ButtonText,
			Qt::white
	);
	darkPalette.setColor(
			QPalette::Disabled,
			QPalette::ButtonText,
			QColor(127,127,127)
	);
	// very different from WindowText and contrasts well with eg. Dark
	darkPalette.setColor(
			QPalette::BrightText,
			color1
	);
	// selected(marked) items:
	darkPalette.setColor(
			QPalette::Highlight,
			color1
	);
	darkPalette.setColor(
			QPalette::HighlightedText,
			Qt::white
	);
	/*
	darkPalette.setColor(
			QPalette::Disabled,
			QPalette::Highlight,
			QColor(80,80,80)
	);
	darkPalette.setColor(
			QPalette::Disabled,
			QPalette::HighlightedText,
			QColor(127,127,127)
	);
	*/
	// Bevels:
	darkPalette.setColor(
			QPalette::Dark,
			QColor(35,35,35)
	);
	darkPalette.setColor(
			QPalette::Shadow,
			QColor(20,20,20)
	);

	darkPalette.setColor(
			QPalette::Link,
			color1
	);

	qApp->setPalette(darkPalette);
}
