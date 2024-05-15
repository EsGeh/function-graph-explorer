#pragma once

#include "fge/model/sampled_func_collection.h"
#include "function_collection.h"
#include "function_collection_impl.h"
#include <future>
#include <memory>
#include <optional>


struct NodeInfo:
	public FunctionCollectionWithInfo::NodeInfo
{
	bool isPlaybackEnabled = false;
	double volumeEnvelope = 1;
	SamplingSettings samplingSettings;
	mutable std::shared_ptr<Cache> cache;
};

using AudioCallback = std::function<void(
		const PlaybackPosition position,
		const uint samplerate
)>;



class SampledFunctionCollectionImpl:
	public SampledFunctionCollectionInternal,
	private FunctionCollectionImpl
{
	public:
		using LowLevel = FunctionCollectionImpl;
		using SampledFunctionCollectionInternal::Index;
	public:
		SampledFunctionCollectionImpl(
				const SamplingSettings& defSamplingSettings,
				AudioCallback audioCallback 
		);
	
		// Size:
		uint size() const override { return LowLevel::size(); }
		void resize(const uint size) override { LowLevel::resize(size); }

		// Read entries:
		virtual FunctionParameters get(
				const size_t index
		) const override;

		virtual MaybeError getError(
				const Index index
		) const override;

		// Set Entries:

		virtual MaybeError set(
				const Index index,
				const QString& formula,
				const ParameterBindings& parameters,
				const StateDescriptions& stateDescriptions
		) override;

		virtual MaybeError setParameterValues(
				const Index index,
				const ParameterBindings& parameters
		) override;


		/***************
		 * Sampling
		 ***************/

		// general settings:
		virtual SamplingSettings getSamplingSettings(
				const Index index
		) override;
		virtual void setSamplingSettings(
				const Index index,
				const SamplingSettings& value
		) override;

		// sampling for visual representation:
		virtual ErrorOrValue<std::vector<std::pair<C,C>>> getGraph(
				const Index index,
				const std::pair<T,T>& range,
				const unsigned int resolution
		) const override;

		// sampling for audio:
		virtual bool getIsPlaybackEnabled(
				const Index index
		) const override;
		virtual void setIsPlaybackEnabled(
				const Index index,
				const bool value
		) override;
		virtual void valuesToBuffer(
				std::vector<float>* buffer,
				const PlaybackPosition position,
				const unsigned int samplerate
		) override;

		virtual double getMasterVolume() const override;
		virtual void setMasterVolume(const double value) override;

	public:
		::NodeInfo* getNodeInfo( const Index index ) {
			return static_cast<::NodeInfo*>(LowLevel::getNodeInfo(index));
		}

	private:
		float audioFunction(
				const PlaybackPosition position,
				const uint samplerate
		);
	private:
		virtual std::shared_ptr<LowLevel::NodeInfo> createNodeInfo(
				const Index index,
				std::shared_ptr<Function> maybeFunction
		) override;

		void updateCaches( const Index index );
		std::shared_ptr<Cache> updateCache( std::shared_ptr<Function> maybeFunction );

		const ::NodeInfo* getNodeInfoConst( const Index index ) const {
			return static_cast<::NodeInfo*>(LowLevel::getNodeInfo(index));
		}

	private:
		SamplingSettings defSamplingSettings;
		AudioCallback audioCallback;
		double masterVolume = 1;
};


/* Utilities */

C getWithResolution(
		std::shared_ptr<Function> function,
		const C& x,
		const SamplingSettings& samplingSettings,
		std::shared_ptr<Cache> cache
);

C interpolate(
		const C& x,
		const std::vector<C> ys,
		const int shift,
		const uint resolution

);

int xToRasterIndex(
		const C& x,
		const uint resolution
);

C rasterIndexToY(
		std::shared_ptr<Function> function,
		int x,
		const uint resolution
);
