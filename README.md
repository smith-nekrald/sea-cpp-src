# C++ sea decision core

The core of the sea cargo decision system is written in C++. The entry point function is located in `main.cpp`. 
In what follows, we describe the organization in further detail.


## Code style notes

The implementation of this core module is provided in C++. The code style can be described as "Yandex Java-based 
C++ Code style" for those familiar with this search engine company. Otherwise, one may assume java-like naming 
transferred into C++ design. Minor inconsistencies and uncertainties does not matter much for a repository with
research code.

The code definitions in headers `*.h` are documented in a format compatible to Doxygen automatic documentation. The 
implementation files `*.cpp` are self-documented. Doxygen-based documentation generation is available in a top-level
module, called `system`, and is described in the correspoding repository.


## General overview

The core decision repository consists of several components. The data-based component specifies data structures and 
the input-output interface. The functional component is based on algorithmic abstraction and protocol interaction. 
The protocol specifies the format of the decision, the format of the action, and the interaction process. A protocol 
essentially defines the interaction between an algorithm and the data. An algorithm is yet another abstraction. One 
may define the algorithmic interface directly or work through the strategic interface. Strategic Algorithm abstraction 
means  specifying two strategies: the Allotment Strategy for decisions related to long-term contracts, and the Spot 
Market Strategy for decisions related to short-term contracts. 

Allotment and Spot strategies essentially implement the interface methods, which makes them applicable within a 
Strategic Algorithm. But the conceptual functionality is delegated to backends, which are allowed to have a free 
form, but should provide all necessary methods and behaviors further requested by strategies or algorithm interface
directly. Launch and evaluation happen through Analyzer, Launcher, Estimator, Validator, and Evaluator, where the necessary 
Statistics are accumulated. 

Besides that, there are components that allow smart logging and memory management. Smart logging is actual, works, 
and helps significantly. The story behind the memory management interface and functions is more interesting. 
Initially, their purpose was to allow manual-based dumping of data structures into the hard drive due to RAM 
limitations. Dump happened either through manually-implemented protocols or through a specially designed library, 
Cereal. Right now, that functionality does not remain relevant, but we need to maintain interfaces for consistency. 
I.e., maybe the memory management works, maybe not, quite a long time has passed, and the functionality has not 
been tested in the updated library version.


## Parts of the file structure

We will next discuss implementation based on the logical components and their relative paths from the repository root.
1. Basic data structures, input-output, and reader interface. Correspond to the parts located in `input` and `reader`. 
2. Memory management abstraction. 
3. Protocol entities and interface. Correspond to the parts implemented in the folder `interaction`.
4. Algorithmic interface, state of the algorithm, strategic algorithm, and baseline (greedy) algorithm. 
Available in the folder `algorithm`.
5. Strategies: interface and implementations. The corresponding source code is located in the `strategy` subfolder.
Allotment strategy interface, spot strategy interface. Abstract allotment strategy, abstract spot strategy. 
Implementations: Benders' allotment strategy, IPOPT allotment strategy, IPOPT spot strategy, hybrid spot strategy, 
Lagrangian relaxation spot strategy. There are also special cases without allotments or with zero spot market activity. 
6. Evaluation: launching algorithms, collecting statistics, and forming outputs. The components involved are launcher, 
estimator, evaluator, validator, and analyzer. The corresponding folders are `evaluation` and `sense`.
7. Backends: real logic behind the interfaces. Free format is allowed; the description is the most advanced. 
The corresponding source code folder is `backend`.
8. Logging. Interface and configuration, correspond to folders `logging` and `conf`.
9. More detailed entry point description.


## Basic data structures and input-output

Input is represented in two types of data: input data, representing the parameters, and market data, 
representing the sample of market behavior. The input data information is represented by `InputData`, 
described in `input/input_data.h`. There are also the definitions of substructures that represent the 
building blocks of `InputData`. More precisely, `ShowRate`, `Demand`, `Port`, `Node`, `Vessel`, `Arc`,
`Itinerary`, `AllotmentEntry`, `Allotment`, `Event`. The concept correspond to the description in the 
paper and physical intuition. Such information is provided to algorithms and backends for estimating 
inner parameters and providing responses on requests.

The market data information is represented by `MarketData`, described in `input/market_data.h`. 
Essentially, market data represents the samples of market realizations, and includes exact demands, 
willingness to pay, shows and cancellations.

Precomputed values and information built on top of `InputData` are provided in `InputLinks`, with 
implementation available at `input/input_links.h`. The function for filling a freshly built 
`InputLinks` from `InputData` is called `createInputLinks(const InputData& input, InputLinks* links)`. 

Input-output functions for `InputData`, `InputLinks` and `MarketData` are described in `input/io_functions.h`.
Serialization for `InputLinks` happends through C++ serialization library `cereal`. For `InputData` and 
`MarketData`, only reading is implemented, because output files already exist and provided at input. 
The corresponding methods rely on readers defined at `reader` folder.

Template reading functions are described in `reader/common_reader.hpp`. General reader interface is 
provided in `reader/ireader.h`. Reader for input data is defined in `reader/input_reader.h`, and 
reader for market data is defined in `reader/market_reader.h`.


## Memory management abstraction

Memory management abstraction corresponds to the file `manager.h`. The structure capable of manual 
memory management is called `DataProxy`. However, there were scenarios where manual memory management
was needed, and were scenarios where manual memory management was not necessary. To easily handle
this difference, another template structure was introduced, `DataManager`. Essentially, wrapper
around `DataProxy`, that allows to switch on and off the memory management functionality.

These two structures were relevant when experiments were handled on machines with large RAM 
availability, and even higher RAM demands. Later, the scale has been reduced, machines with 
large RAM became unavailable, and therefore, the memory management structures became irrelevant. 
However, due to the maturity status of the library, the interface had to stay the previously 
defined form.

All data is transmitted through smart pointers, mostly of the type `shared_ptr<DataManager<Entity>>`.
Very often, the size of each entity is rather large, and no plans to modify after building or reading.
Therefore, such implementation decision has been chosen. The corresponding input-related pointers
are called `InputManagerPtr`, `LinksManagerPtr`, `MarketManagerPtr`. For constant versions, prefix
`Const` is added. Pointers are also defined for certain entitities defined at protocol and backend.


## Protocol entities 

Let us now discuss the folder `interaction`. The file `protocol.h` defines interaction entities, 
`Decision` and `Action`. Intuitively, `Decision` is the structure to report the decision at the
considered period, and `Action` is the structure to receive the response of the environment 
with the information that has been revealed between response in the previous period and request 
in the current period. According to the previously explained conventions, due to the RAM size 
requirements, these structures are transmitted in smart pointers of the type `shared_ptr`. 
Typically, the smart pointers are written around memory managers. The interface of sending 
actions and decisions back-and-forth is provided through an entity `IInteractor` defined in 
`iinteractor.h`. Template functions related to serialization and writing to output streams are 
defined in `protocol.hpp`.


## Algorithmic interface

Algorithm interface is provided in folder `algorithm`, in the file `ialgorithm.h`. Essentially,
`IAlgorithm` inherits from `IInteractor`, but also requires methods reporting the name of the
algorithm and allowing to reset the state of the inner structures. State of the interaction 
is described by the structure `State` from the header file `state.h`, the same header also 
defines the initialization method. Doxygen-based documentation elaborates on the nature of 
the fields of the structure and related methods.

First implementation of the algorithm is provided in header `greedy_algorithm.h`, which defines
the baseline. The baseline, `GreedyAlgorithm`, implements the interface defined in `IAlgorithm`
and is initialized by `GreedyConfig`. The other implementation of the algorithm interface is 
throught the `StrategicAlgorithm`, with header `strategic_algorithm.h`. The strategic algorithm
relies on two components, `IAllotmentStrategy` for decisions around the long-term market, and
`ISpotStrategy` for decisions in spot market. The implementations for the described interfaces
are provided in the `strategy` subfolder.


## Strategies: interface and implementations

Strategies are located int the `strategy` subfolder, and essentially define the components later
requested through `StrategicAlgorithm`. Interface for long-term market strategy, `IAllotmentStrategy`,
is provided in `iallotment_strategy.h`. Interface for spot market strategy, `ISpotMarketStrategy`,
is provided in `ispot_market_strategy.h`. 


### Spot Market Strategies

Here we further focus on spot market strategies. The header `abstract_spot_market_strategy.h` defines 
abstract spot market strategy: not only interface, but some members and some implementations, common 
for every inheriting spot market strategy. Abstract spot market strategy is configured through 
SpotMarketStrategyConfig, which is a common configuration containing fields to handle possible backends.

The following spot market strategies are implemented (and inherit from abstract):

1. Lagrangian Relaxation, available in `strategy/lr_spot/lr_spot_market_strategy.h`
2. Ipopt spot strategy, available in `strategy/ipopt/ipopt_spot_strategy.h`
3. Hybrid spot market strategy, available in `strategy/hybrid/hybrid_spot_market_strategy.h`. Here the idea 
is to bring dual variables from Ipopt constraints directly into Lagrangian Relaxation without running UFGM.
4. Nospot strategies, i.e. directly ensuring no spot market activity. Available in `strategy/no_spot/lt_spot_strategy.h`.

### Allotment Market Strategies

The header `abstract_allotment_strategy.h` defines abstract allotment strategy. Configuration happens through
a dedicated AllotmentStrategyConfig, which holds fields for possible backends that can be further referenced.

The following allotment strategies are implemented (inheriting from abstract):
1. Ipopt allotment strategy. Described in `strategy/ipopt/ipopt_allotment_strategy.h`. Based on calling Ipopt 
solver from a correpsonding backend.
2. Benders allotment strategy, described in `strategy/benders/benders_lr_allotment_strategy.h`. Approximate 
method based on Benders decomposition with several educated guesses, proved to be ineffective due to non-canonical 
structure. Not reported in the final paper version.
3. No-spot aware allotment strategy, available at `strategy/no_spot/nospot_aware.h`. Making ipopt-based allotment
decisions expecting no activity in spot market policy later.
4. Zero allotment strategy, no allotments are accepted, but later, spot market is allowed. Located at 
`strategy/zero/zero_allotment_strategy.h`
5. Null allotment strategy. No allotments, and at expected initialization no spot market in the future. Described
in `strategy/null/null_allotment_strategy.h`


Inner organization is better described with Doxygen-based documentation.

## Acknowledgements

The major contribution to the model and implementation is attributed to the paper co-authors: 
Aliaksandr Nekrashevich, Mikhail Y. Kovalyov, Mikhail Nediak, Genrikh Levin, Yuri Levin, 
Guang Li. Several individuals have been working on the implementation besides the co-authors. 
Among them are Henadzi Klimuk, Maxym Sporyshev, and Alexandra Zhihareva. Their participation 
has been voluntarily terminated based on other priorities and the huge time investment that 
became obvious in the middle of the path between the initial draft and the first submitted 
version.


