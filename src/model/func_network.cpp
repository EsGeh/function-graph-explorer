#include "fge/model/func_network.h"
#include "fge/model/func_network_impl.h"

std::shared_ptr<FuncNetwork> funcNetworkFabric()
{
	return std::shared_ptr<FuncNetwork>( new FuncNetworkImpl() );
}
