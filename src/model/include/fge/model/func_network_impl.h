#pragma once

#include "fge/model/func_network.h"
#include <memory>


template <class NodeInfo>
class FuncNetworkImpl:
	public FuncNetworkWithInfo<NodeInfo>
{
	public:
		using Base = FuncNetworkWithInfo<NodeInfo>;
		using Index = Base::Index;

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
			std::shared_ptr<NodeInfo> info = std::make_shared<NodeInfo>();
			// bool isAudioEnabled = false;
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

template <class NodeInfo>
FuncNetworkImpl<NodeInfo>::FuncNetworkImpl()
	: constants( symbols() )
{}

template <class NodeInfo>
uint FuncNetworkImpl<NodeInfo>::size() const
{
	return entries.size();
}

template <class NodeInfo>
FunctionOrError FuncNetworkImpl<NodeInfo>::get(const Index index) const
{
	auto entry = entries.at( index );
	return entry->functionOrError
	.transform_error([](auto invalidEntry) {
			return invalidEntry.error;
	});
}

template <class NodeInfo>
FunctionParameters FuncNetworkImpl<NodeInfo>::getFunctionParameters(const uint index) const
{
	auto entry = entries.at( index );
	if( entry->functionOrError ) {
		auto function = entry->functionOrError.value();
		return FunctionParameters{
			.formula = function->toString(),
			.parameters = function->getParameters(),
			.stateDescriptions = function->getStateDescriptions()
		};
	}
	else {
		auto error = entry->functionOrError.error();
		return error.parameters;
	}
}

/*
bool FuncNetworkImpl<NodeInfo>::getIsPlaybackEnabled(
		const uint index
) const
{
	return entries.at( index )->isAudioEnabled;
}

void FuncNetworkImpl<NodeInfo>::setIsPlaybackEnabled(
		const uint index,
		const bool value
)
{
	auto entry = entries.at( index );
	entry->isAudioEnabled = value;
}
*/


template <class NodeInfo>
void FuncNetworkImpl<NodeInfo>::resize( const uint size ) {
	const auto oldSize = entries.size();
	if( size < oldSize ) {
		entries.resize( size );
	}
	else if( size > oldSize ) {
		for( uint i=oldSize; i<size; i++ ) {
			auto entry = std::shared_ptr<NetworkEntry>(new NetworkEntry {
					.functionOrError = std::unexpected(InvalidEntry{
						.error = "not yet initialized",
						.parameters = FunctionParameters{
								.formula = (i>0)
									? QString("%1(x)").arg( functionName(i-1) )
									: "cos( 2pi * x )"
						}
					})
			});
			entries.push_back( entry );
		}
		updateFormulas(oldSize, {});
	}
	assert( entries.size() == size );
}

template <class NodeInfo>
MaybeError FuncNetworkImpl<NodeInfo>::set(
		const Index index,
		const FunctionParameters& parameters
) {
	// assert( index < size() );
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<index; i++ ) {
		auto entry = entries.at(i);
		if( entry->functionOrError.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
			);
		}
	}
	updateFormulas( index, parameters );
	if( !get(index) ) {
		return get(index).error();
	}
	return {};
}

template <class NodeInfo>
MaybeError FuncNetworkImpl<NodeInfo>::setParameterValues(
		const Index index,
		const ParameterBindings& parameters
)
{
	FunctionOrError functionOrError = get( index );
	if( !functionOrError ) {
		return functionOrError.error();
	}
	auto function = functionOrError.value();
	for( auto [key, val] : parameters ) {
		auto maybeError = function->setParameter( key, val );
		if( maybeError ) {
			return maybeError.value();
		}
	}
	return {};
}

template <class NodeInfo>
void FuncNetworkImpl<NodeInfo>::updateFormulas(
		const size_t startIndex,
		const std::optional<FunctionParameters>& parameters
)
{
	Symbols functionSymbols;
	/* dont change entries
	 * before start index
	 * but add their
	 * function symbols
	 */
	for( size_t i=0; i<startIndex; i++ ) {
		auto entry = entries.at(i);
		if( entry->functionOrError.has_value() ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
			);
		}
	}
	/* update entries
	 * from startIndex:
	 */
	for( size_t i=startIndex; i<entries.size(); i++ ) {
		auto entry = entries.at(i);
		FunctionParameters params = getFunctionParameters(i);
		if( i==startIndex && parameters ) {
			params = parameters.value();
		}
		entry->functionOrError = formulaFunctionFactory(
				params.formula,
				params.parameters,
				params.stateDescriptions,
				{
					constants,
					functionSymbols
				}
		).transform_error([params](auto error){
			return InvalidEntry{
				.error = error,
				.parameters = params
			};
		});
		if( entry->functionOrError ) {
			functionSymbols.addFunction(
					functionName( i ),
					entry->functionOrError.value().get()
			);
		}
	}
}

template <class NodeInfo>
NodeInfo* FuncNetworkImpl<NodeInfo>::getNodeInfo(
		const Index index
) const
{
	return entries.at(index)->info.get();
}

template <class NodeInfo>
std::shared_ptr<FuncNetworkWithInfo<NodeInfo>> funcNetworkWithInfoFabric()
{
	return std::make_shared<FuncNetworkImpl<NodeInfo>>();
}
