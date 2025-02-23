#include "resources.h"


Resources loadResources() {
	Resources resources;
	using std::filesystem::path;
	path resourcesDir = defaultResourcesDir;
	if( auto fromEnv = std::getenv( RES_DIR_ENV_VAR.c_str() ) ) {
		resourcesDir = fromEnv;
	}
	// load tips:
	for( const auto& entry : std::filesystem::directory_iterator( resourcesDir / "tips") ) {
		if( entry.path().extension() != ".md" ) { continue; }
		std::ostringstream strstr;
		std::ifstream file( entry.path(), std::ios::out);
		if( ! file.is_open() ) {
			throw std::runtime_error( "failed to load resources" );
		}
		strstr << file.rdbuf();
		resources.tips.push_back( QString( strstr.str().c_str() ) );
	}
	// load help:
	{
		const auto path = resourcesDir / "help" / "help.md";
		std::ostringstream strstr;
		std::ifstream file( path, std::ios::out);
		if( ! file.is_open() ) {
			throw std::runtime_error( "failed to load resources" );
		}
		strstr << file.rdbuf();
		resources.help = QString( strstr.str().c_str() );
	}
	return resources;
}
