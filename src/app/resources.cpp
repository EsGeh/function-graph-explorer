#include "resources.h"

#include <QFile>
#include <QDirIterator>
#include <qdir.h>


Resources loadResources() {
	Resources resources;
	resources.tips = {};
	// using std::filesystem::path;
	QString resourcesDir = defaultResourcesDir;
	if( auto fromEnv = std::getenv( RES_DIR_ENV_VAR.c_str() ) ) {
		resourcesDir = fromEnv;
	}
	// resources.tips:
	{
		QDirIterator it( resourcesDir + "/tips", QDirIterator::Subdirectories );
		while( it.hasNext() )
		{
			const auto path = it.next();
			if( !path.endsWith( ".md" ) ) { continue; }
			QFile file(path);
			if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
				throw std::runtime_error( "failed to load resources" );
			QTextStream stream(&file);
			resources.tips.push_back( stream.readAll() );
		}
	}
	// resources.help:
	{
		QFile file( resourcesDir + "/help/help.md");
		if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
			throw std::runtime_error( "failed to load resources" );
		QTextStream stream(&file);
		resources.help = stream.readAll();
	}
	return resources;
}
