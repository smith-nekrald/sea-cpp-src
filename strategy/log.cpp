#include "abstract_allotment_strategy.h"
#include "abstract_spot_market_strategy.h"

namespace sea {
namespace strategy {

void AbstractAllotmentStrategy::logCreatedAbstractAllotmentStrategy() const {
    logging::getAllotmentStrategyLogger(config.type.value()).info(
            "AbstractAllotmentStrategy created.");
}

void AbstractAllotmentStrategy::logHardResetInAbstractAllotmentStrategy() const {
    logging::getAllotmentStrategyLogger(config.type.value()).debug(
            "AbstractAllotmentStrategy::hardReset() is called.");
}

void AbstractSpotMarketStrategy::logCreated() const {
    logging::getSpotStrategyLogger(config.type.value()).info(
            "Created AbstractSpotMarketStrategy.");
}

void AbstractSpotMarketStrategy::logCalledMakeDecision() const {
    logging::getSpotStrategyLogger(config.type.value()).debug(
            "AbstractSpotMarketStrategy::makeDecision is called.");
}

void AbstractSpotMarketStrategy::logFinishedMakeDecision() const {
    logging::getSpotStrategyLogger(config.type.value()).debug(
            "AbstractSpotMarketStrategy::makeDecision finished.");
}

} // namespace strategy
} // namespace sea

