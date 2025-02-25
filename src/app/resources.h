#pragma once

#include "fge/view/mainwindow.h"


const std::string RES_DIR_ENV_VAR = "FGE_RES_DIR";
const QString defaultResourcesDir = ":resources";

Resources loadResources();
