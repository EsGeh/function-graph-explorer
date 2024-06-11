#include "fge/model/model.h"
#include "fge/model/model_impl.h"


std::shared_ptr<Model> modelFactory(
		const SamplingSettings& defSamplingSettings
) {
	return std::shared_ptr<Model>(new ModelImpl(defSamplingSettings));
}
