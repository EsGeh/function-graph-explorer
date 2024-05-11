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

class FunctionCollection
{
	public:
		using Index = uint;
	public:
		virtual ~FunctionCollection() {};
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
};

class FunctionCollectionWithInfo:
	virtual public FunctionCollection
{
	public:
		using Index = FunctionCollection::Index;
		struct NodeInfo {};
	public:
		virtual NodeInfo* getNodeInfo(
				const Index index
		) const = 0;

		virtual std::shared_ptr<NodeInfo> createNodeInfo() {
			return std::shared_ptr<NodeInfo>(new NodeInfo{});
		};
};
