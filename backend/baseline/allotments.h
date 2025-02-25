#include "../backend_general.h"

#include <vector>

namespace sea {
namespace backend {

struct AllotmentSorterConfig {
    ConstInputManagerPtr inputManager;
    ConstLinksManagerPtr linksManager;
};

class IAllotmentSorter {
public:
    IAllotmentSorter() {};
    virtual std::vector<int> selectOrder() = 0;
    virtual ~IAllotmentSorter() {};
};


class TrivialOrder: public IAllotmentSorter {
};

class ByEffectiveCapacity: public IAllotmentSorter {
};

class ByEffectiveProfit: public IAllotmentSorter {
};

class ByProfitPerCapacity: public IAllotmentSorter {
};

class CompositeSorter: public IAllotmentSorter {
};

class IAllotmentOrderMetric {
public:
    IAllotmentOrderMetric() {};
    virtual double score(std::vector<int>& allotmentOrder) = 0;
    virtual ~IAllotmentOrderMetric() {};
};


} // namespace backend
} // namespace sea
