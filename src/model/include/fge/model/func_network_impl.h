#pragma once

#include "fge/model/func_network.h"
#include <memory>


class FuncNetworkImpl:
	public FuncNetworkWithInfo
{
	public:
		using Base = FuncNetworkWithInfo;
		using Index = Base::Index;
		using NodeInfo = FuncNetworkWithInfo::NodeInfo;

	private:
		struct InvalidEntry
		{
			QString error;
			FunctionParameters parameters;
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
		FuncNetworkImpl();

		virtual uint size() const override;
		virtual void resize( const uint size ) override;

		virtual std::expected<std::shared_ptr<Function>,Error> get(const Index index) const override;
		virtual MaybeError set(
				const Index index,
				const FunctionParameters& parameters
		) override;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;

		virtual FunctionParameters getFunctionParameters(const uint index) const override;

		virtual NodeInfo* getNodeInfo(
				const Index index
		) const override;

	private:
		void updateFormulas(
				const size_t startIndex,
				const std::optional<FunctionParameters>& parameters
		);
	private:
		Symbols constants;
		std::vector<std::shared_ptr<NetworkEntry>> entries;
};

Symbols symbols();
inline QString functionName( const size_t index ) {
	return QString("f%1").arg( index );
}
