#pragma once

#include "../ipopt/index_map.h"
#include "../../input/input_links.h"
#include "../../input/input_data.h"


namespace sea {
namespace backend {

struct DcpIndexMap: IpoptIndexMap {
    std::vector<std::vector<unsigned>> timeItineraryToZi;
    unsigned demandZCount = 0; // First <demandZCount> variables from variableCount are z_i
};

void dcpCreateIndexMap(const InputData& input, DcpIndexMap& indexMap);

} // namespace backend
} // namespace sea
