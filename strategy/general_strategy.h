#pragma once

#include "abstract_allotment_strategy.h"
#include "abstract_spot_market_strategy.h"

#include "benders/benders_lr_allotment_strategy.h"
#include "ipopt/ipopt_allotment_strategy.h"
#include "det_cut_plane/dcp_allotment_strategy.h"

#include "ipopt/ipopt_spot_strategy.h"
#include "lr_spot/lr_cutting_plane_spot_market_strategy.h"
#include "hybrid/hybrid_spot_market_strategy.h"

#include "zero/zero_allotment_strategy.h"
#include "null/null_allotment_strategy.h"

#include "no_spot/lt_spot_strategy.h"
#include "no_spot/nospot_aware.h"

