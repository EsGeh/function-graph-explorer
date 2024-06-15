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
		struct ValidEntry
		{
			std::shared_ptr<Function> function;
			ParameterDescriptions parameterDescriptions;
		};
		struct InvalidEntry
		{
			QString error;
			FunctionInfo functionInfo;
			SamplingSettings samplingSettings;
		};

		using FunctionOrInvalid = std::expected<
			ValidEntry,
			InvalidEntry
		>;

		struct NetworkEntry
		{
			FunctionOrInvalid functionOrError;
			std::shared_ptr<NodeInfo> info = nullptr;
		};

	public:
		FunctionCollectionImpl(
				const SamplingSettings& defSamplingSettings
		);

		virtual uint size() const override;
		virtual void resize( const uint size ) override;

		virtual std::expected<std::shared_ptr<Function>,Error> getFunction(const Index index) const override;
		virtual MaybeError set(
				const Index index,
				const FunctionInfo& functionInfo
		) override;
		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;

		virtual FunctionInfo getFunctionInfo(const uint index) const override;

		virtual NodeInfo* getNodeInfo(
				const Index index
		) const override;

		// Sampling Settings:
		virtual SamplingSettings getSamplingSettings(
				const Index index
		) const override;
		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) override;

	private:
		void updateFormulas(
				const size_t startIndex,
				const std::optional<FunctionInfo>& functionInfo
		);
	private:
		Symbols constants;
		SamplingSettings defSamplingSettings;
		std::vector<std::shared_ptr<NetworkEntry>> entries;
};

Symbols symbols();
inline QString functionName( const size_t index ) {
	return QString("f%1").arg( index );
}
