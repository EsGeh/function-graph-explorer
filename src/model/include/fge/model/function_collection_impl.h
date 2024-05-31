#pragma once

#include "fge/model/function_collection.h"
#include <memory>


class FunctionCollectionImpl:
	public FunctionCollectionWithInfo
{
	public:
		using Base = FunctionCollectionWithInfo;
		using Index = Base::Index;
		using NodeInfo = FunctionCollectionWithInfo::NodeInfo;

	private:
		struct InvalidEntry
		{
			QString error;
			FunctionInfo parameters;
		};

		using FunctionOrInvalid = std::expected<
			std::shared_ptr<Function>,
			InvalidEntry
		>;

		struct NetworkEntry
		{
			FunctionOrInvalid functionOrError;
			std::shared_ptr<NodeInfo> info = nullptr;
		};

	public:
		FunctionCollectionImpl();

		virtual uint size() const override;
		virtual void resize( const uint size ) override;

		virtual std::expected<std::shared_ptr<Function>,Error> get(const Index index) const override;
		virtual MaybeError set(
				const Index index,
				const FunctionInfo& parameters
		) override;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;

		virtual FunctionInfo getFunctionInfo(const uint index) const override;

		virtual NodeInfo* getNodeInfo(
				const Index index
		) const override;

	private:
		void updateFormulas(
				const size_t startIndex,
				const std::optional<FunctionInfo>& parameters
		);
	private:
		Symbols constants;
		std::vector<std::shared_ptr<NetworkEntry>> entries;
};

Symbols symbols();
inline QString functionName( const size_t index ) {
	return QString("f%1").arg( index );
}
