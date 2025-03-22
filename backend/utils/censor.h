#pragma once

#include "../manager.h"
#include "../input/input_data.h"
#include "../algorithm/state.h"

namespace sea {
namespace backend {

class SpotPricingCensor {
public:
    SpotPricingCensor(ConstInputManagerPtr inputManager, ConstLinksManagerPtr linksManager);
    void correctPricing(
            const InputData::Event& event, const State& state, DecisionManagerPtr decisionManager);
    virtual ~SpotPricingCensor() {};
private:
    /**
     * @brief Computes expected unit profit for a given place in pricing event.
     * The place index defines itinerary id through event.relatedItineraryIds[placeIdx] and
     * corresponding demand through event.demands[placeIdx]. Price is retrieved from decision.
     *
     * @param event The pricing event.
     * @param placeIdx The index of the place in the pricing event.
     * @return Expected unit transportation profit for the correspoinding itinerary and demand.
     */
    double computeExpectedUnitProfit(const InputData::Event& event, unsigned placeIdx) const;
    /** @brief Sorts place ids from pricing event in descending order of expected unit profit.
     * The place index defines itinerary id through event.relatedItineraryIds[placeIdx] and
     * corresponding demand through event.demands[placeIdx]. Price is retrieved from decision.
     *
     * @param event The pricing event.
     * @return Vector of place ids sorted in descending order of the
     *      expected unit transportation profit.
     */
    std::vector<unsigned> preparePricingPlaceIds(const InputData::Event& event) const;


};

} // namespace backend
} // namespace sea

