#include "fge/view/keybindings.h"
#include <QShortcut>
#include <qnamespace.h>


void addMainWindoKeycodes(MainWindow* view) {
	for( uint i=0; i<9; i++) {
		new QShortcut( QKeySequence( QString("Ctrl+%1").arg(i+1) ), view, [view,i]{ view->focusFunction(i); } );
	}
	new QShortcut( QKeySequence( "Ctrl+Alt++" ), view, [view]{ view->addFunction(); });
	new QShortcut( QKeySequence( "Ctrl+Alt+-" ), view, [view]{ view->removeFunction(); });
	new QShortcut( QKeySequence( "Ctrl+Space" ), view, [view]{ view->togglePlaybackEnabled(); });
};

void addFunctionViewKeys(FunctionView* view)
{
	new QShortcut( QKeySequence( "Ctrl+F" ), view, [view]{ view->focusFormula(); }, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+G" ), view, [view]{ view->focusGraph(); }, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+Return" ), view, [view]{ view->openDisplayDialog(); }, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+P" ), view, [view]{ view->togglePlaybackEnabled(); }, Qt::WidgetWithChildrenShortcut );
}

void addGraphViewKeyCodes(GraphView* view)
{
	// translation:
	new QShortcut( QKeySequence( Qt::Key_Left ), view, [view]{
			view->moveView({-1,0});
			emit view->viewChanged();
	}, Qt::WidgetShortcut );
	new QShortcut( QKeySequence( Qt::Key_Right ), view, [view]{
			view->moveView({+1,0});
			emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( Qt::Key_Down ), view, [view]{
			view->moveView({0,-1});
			emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( Qt::Key_Up ), view, [view]{
			view->moveView({0,+1});
			emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	// Zoom:
	new QShortcut( QKeySequence( "Ctrl+0" ), view, [view]{
				view->resetView();
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl++" ), view, [view]{
				view->zoomView({-1,-1});
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+-" ), view, [view]{
				view->zoomView({+1,+1});
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+Down" ), view, [view]{
				view->zoomView({0,-1});
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+Up" ), view, [view]{
				view->zoomView({0,+1});
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+Left" ), view, [view]{
				view->zoomView({-1,0});
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
	new QShortcut( QKeySequence( "Ctrl+Right" ), view, [view]{
				view->zoomView({+1,0});
				emit view->viewChanged();
	}, Qt::WidgetWithChildrenShortcut );
}
