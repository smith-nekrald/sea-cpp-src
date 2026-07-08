# Sea Core

The core of the sea cargo decision system is written in C++. The entry point function is located in `main.cpp`. 
The library is organized into the following parts.

# Acknowledgements

The major contribution to the model and implementation is attributed to the paper co-authors: Aliaksandr Nekrashevich, 
Mikhail Y. Kovalyov, Mikhail Nediak, Genrikh Levin, Yuri Levin, Guang Li. Several individuals have been working on the 
implementation besides the co-authors. Among them are Henadzi Klimuk, Maxym Sporyshev, and Alexandra Zhihareva. Their 
participation has been voluntarily terminated based on other priorities and the huge time investment that became obvious in
the middle of the path between the initial version and the first submitted version.

# General overview

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

# Parts of the file structure

We will next discuss implementation based on the logical components and their relative paths from the repository root.
1. Basic data structures, input-output, and reader interface. Correspond to the parts located in `input` and `reader`. 
2. Memory management abstraction. 
3. Protocol entities and interface. Correspond to the parts implemented in the folder `interaction`.
4. Algorithmic interface, state of the algorithm, strategic algorithm, and baseline (greedy) algorithm. 
Available in the folder `algorithm`.
5. Strategies: interface and implementations. The corresponding source code is located in the `strategy` subfolder.
Allotment strategy interface, spot strategy interface. Abstract allotment strategy, abstract spot strategy. Implementations: 
Benders' allotment strategy, IPOPT allotment strategy, IPOPT spot strategy, hybrid spot strategy, Lagrangian relaxation spot
strategy. There are also special cases without allotments or with zero spot market activity. 
6. Evaluation: launching algorithms, collecting statistics, and forming outputs. The components involved are launcher, 
estimator, evaluator, validator, and analyzer. The corresponding folders are `evaluation` and `sense`.
7. Backends: real logic behind the interfaces. Free format is allowed; the description is the most advanced. The corresponding
source code folder is `backend`.
8. Logging. Interface and configuration, correspond to folders `logging` and `conf`.
9. More detailed entry point description.
   
