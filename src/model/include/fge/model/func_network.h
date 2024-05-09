#pragma once

#include "fge/model/function.h"
#include "fge/shared/data.h"
#include <memory>


struct FunctionParameters
{
	QString formula;
	ParameterBindings parameters;
	StateDescriptions stateDescriptions;
};

using FunctionOrError =
	std::expected<std::shared_ptr<Function>,Error>;

class FuncNetwork
{
	public:
		using Index = uint;
	public:
		virtual ~FuncNetwork() {};
		// Size
		virtual uint size() const = 0;
		virtual void resize( const uint size ) = 0;

		// Read / write entries:
		virtual FunctionOrError get(const uint index) const = 0;

		virtual MaybeError set(
				const Index index,
				const FunctionParameters& parameters
		) = 0;
		virtual FunctionParameters getFunctionParameters(const uint index) const = 0;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) = 0;

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const = 0;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) = 0;
};

std::shared_ptr<FuncNetwork> funcNetworkFabric();
