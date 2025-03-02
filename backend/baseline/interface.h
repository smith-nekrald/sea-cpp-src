#include <vector>
#include <string>

namespace sea {
namespace backend {


class IAllotmentSorter {
public:
    IAllotmentSorter() {};
    virtual std::vector<unsigned> selectOrder() const = 0;
    virtual std::string getName() const = 0;
    virtual ~IAllotmentSorter() {};
};


class IAllotmentOrderMetric {
public:
    IAllotmentOrderMetric() {};
    virtual double score(const std::vector<unsigned>& allotmentOrder) const = 0;
    virtual ~IAllotmentOrderMetric() {};
};

} // namespace backend
} // namespace sea
