#pragma once

#include "fge/model/func_network.h"


class FuncNetworkImpl:
	public FuncNetwork
{
	public:

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
			bool isAudioEnabled = false;
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

		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;

	private:
		void updateFormulas(
				const size_t startIndex,
				const std::optional<FunctionParameters>& parameters
		);
	private:
		Symbols constants;
		std::vector<std::shared_ptr<NetworkEntry>> entries;
};
