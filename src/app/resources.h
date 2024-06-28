#pragma once

#include "fge/view/mainwindow.h"

#include <filesystem>


const std::string RES_DIR_ENV_VAR = "FGE_RES_DIR";
const std::filesystem::path defaultResourcesDir = "resources";

Resources loadResources();
