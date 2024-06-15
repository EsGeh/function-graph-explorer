#pragma once
#include "fge/model/function.h"
#include "fge/shared/data.h"
#include <memory>


struct FunctionInfo
{
	QString formula;
	ParameterBindings parameters;
	ParameterDescriptions parameterDescriptions;
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
		virtual FunctionOrError getFunction(const Index index) const = 0;

		virtual MaybeError set(
				const Index index,
				const FunctionInfo& functionInfo
		) = 0;
		virtual FunctionInfo getFunctionInfo(const uint index) const = 0;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) = 0;

	// Sampling Settings:
	virtual SamplingSettings getSamplingSettings(
			const Index index
	) const = 0;
	virtual void setSamplingSettings(
			const Index index,
			const SamplingSettings& value
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

		virtual std::shared_ptr<NodeInfo> createNodeInfo(
				const Index index,
				std::shared_ptr<Function> maybeFunction
		)
		{
			return std::shared_ptr<NodeInfo>(new NodeInfo{});
		};
};
