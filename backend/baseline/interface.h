#include <vector>

namespace sea {
namespace backend {


class IAllotmentSorter {
public:
    IAllotmentSorter() {};
    virtual std::vector<unsigned> selectOrder() = 0;
    virtual ~IAllotmentSorter() {};
};


class TrivialOrder: public IAllotmentSorter {
};

class ByEffectiveCapacity: public IAllotmentSorter {
};


class ByProfitPerCapacity: public IAllotmentSorter {
};

class IAllotmentOrderMetric {
public:
    IAllotmentOrderMetric() {};
    virtual double score(const std::vector<unsigned>& allotmentOrder) = 0;
    virtual ~IAllotmentOrderMetric() {};
};

} // namespace backend
} // namespace sea
