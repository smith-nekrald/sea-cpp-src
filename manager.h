#pragma once

#include <memory>
#include <string>
#include <chrono>
#include <experimental/filesystem>
#include <cassert>

#include "common.h"
#include "interaction/protocol.h"
#include "backend/ipopt/index_map.h"
#include "backend/det_cut_plane/index_map.h"
#include "backend/lagrangian_relaxation/index.h"
#include "input/io_functions.h"
#include "logging/logging.h"

namespace sea {

template<typename DataType>
class DataProxy {
public:
    DataProxy(std::string aFilePath,
            bool leaveEmpty = false,
            bool useTemporaryFileVal = true)

        : filePath(aFilePath)
        , useTemporaryFile(useTemporaryFileVal)
        , isDumped(!useTemporaryFileVal)
        , isValid(true) {

        logging::getMemoryLogger().debug("DataProxy.Constructor is called.");
        if (leaveEmpty) {
            assert(!useTemporaryFile);
            logging::getMemoryLogger().debug("DataProxy. Leaving empty.");
            logging::getMemoryLogger().debug("DataProxy.Constructor has finished.");
            return;
        }
        if (!useTemporaryFile) {
            loadData();
        } else {
            data = std::make_unique<DataType>();
            logging::getMemoryLogger().debug("DataProxy. Created new data.");
        }
        isEmpty = false;
        logging::getMemoryLogger().debug("DataProxy.Constructor has finished.");

    }
    DataProxy(const DataProxy& proxy)
            : useTemporaryFile(proxy.useTemporaryFile) {
        logging::getMemoryLogger().debug("DataProxy.CopyConstructor is called.");
        assert(proxy.isValid);
        isValid = proxy.isValid;
        isEmpty = proxy.isEmpty;
        isDumped = proxy.isDumped;

        if (!isEmpty) {
            data = std::make_unique<DataType>();
            *data.get() = *proxy.data.get();
        }

        if (!proxy.useTemporaryFile) {
            filePath = proxy.filePath;
        } else {
            filePath = makeUniqueFileName() + "_copy_" + proxy.filePath;
            if (isEmpty) {
                std::experimental::filesystem::copy(proxy.filePath, filePath);
            }
        }
        logging::getMemoryLogger().debug("DataProxy.CopyConstructor has finished.");
    }

    DataType& getData() {
        logging::getMemoryLogger().debug("DataProxy.getData() is called.");
        assert(isValid && !isEmpty);
        logging::getMemoryLogger().debug("DataProxy.getData() has finished.");
        return *data;
    }
    DataType* get() {
        logging::getMemoryLogger().debug("DataProxy.get() is called.");
        assert(isValid && !isEmpty);
        logging::getMemoryLogger().debug("DataProxy.get() has finished.");
        return data.get();
    }

    const DataType& getConstData() const {
        logging::getMemoryLogger().debug("DataProxy.getConstData() is called.");
        assert(isValid && !isEmpty);
        logging::getMemoryLogger().debug("DataProxy.getConstData() has finished.");
        return *data;
    }

    void releaseMemory() {
        logging::getMemoryLogger().debug("DataProxy.releaseMemory() is called.");
        assert(isValid && !isEmpty);
        if (!isDumped) {
            dumpData();
        }
        assert(isDumped);
        data.reset();
        isEmpty = true;
        logging::getMemoryLogger().debug("DataProxy.releaseMemory() has finished.");
    }

    void dumpData() {
        logging::getMemoryLogger().debug("DataProxy.dumpData() is called.");
        assert(isValid && !isEmpty && !isDumped);
        if (useTemporaryFile) {
            writeToFile(filePath, *data);
            isDumped = true;
        }
        assert(isDumped);
        logging::getMemoryLogger().debug("DataProxy.dumpData() has finished");
    }

    void loadData() {
        logging::getMemoryLogger().debug("DataProxy. Called loadData()");
        assert(isValid && isDumped && isEmpty);
        data = std::make_unique<DataType>();
        DataType* rawPointer = data.get();
        loadFromFile(filePath, rawPointer);
        if (useTemporaryFile) {
            removeTemporaryFile();
            isDumped = false;
        }
        isEmpty = false;
        logging::getMemoryLogger().debug("DataProxy.loadData() has finished.");
    }



    void removeTemporaryFile() {
        logging::getMemoryLogger().debug("DataProxy. Called removeTemporaryFile()");
        assert(useTemporaryFile);
        if (std::experimental::filesystem::is_regular_file(filePath)) {
            std::experimental::filesystem::remove(filePath);
        }
        logging::getMemoryLogger().debug("DataProxy.removeTemporaryFile() has finished.");
    }

    void invalidate() {
        logging::getMemoryLogger().debug("DataProxy.invalidate() is called.");
        if (useTemporaryFile) {
            removeTemporaryFile();
        }
        data.reset();
        isValid = false;
        logging::getMemoryLogger().debug("DataProxy.invalidate() has finished.");
    }

    bool empty() {
        logging::getMemoryLogger().debug("DataProxy.empty() is called.");
        assert(isValid);
        logging::getMemoryLogger().debug("DataProxy.empty() has finished.");
        return isEmpty;
    }

    bool dumped() {
        logging::getMemoryLogger().debug("DataProxy.dumped() is called.");
        assert(isValid);
        logging::getMemoryLogger().debug("DataProxy.dumped() has finished.");
        return isDumped;
    }

    bool valid() {
        logging::getMemoryLogger().debug("DataProxy.valid() is called.");
        logging::getMemoryLogger().debug("DataProxy.valid() has finished.");
        return isValid;
    }

    ~DataProxy() {
        logging::getMemoryLogger().debug("DataProxy.destructor() is called.");
        if (useTemporaryFile) {
            removeTemporaryFile();
        }
        logging::getMemoryLogger().debug("DataProxy.destructor() has finished.");
    }


private:
    std::string filePath;
    const bool useTemporaryFile;

    std::unique_ptr<DataType> data;
    bool isEmpty = true;
    bool isDumped;

    bool isValid = true;
};

struct ManagerConfig {
    bool needMemory;
    std::string filePath;
    bool useTemporaryFile;
};

template<typename DataType>
class DataManager {

public:
    DataManager(bool memoryNeed = false)
            : needMemory(memoryNeed) {
        dataProxy = nullptr;
        logging::getMemoryLogger().debug("Empty data manager created.");
    }

    DataManager(const ManagerConfig& config)
            : needMemory(config.needMemory) {
        if (!config.useTemporaryFile && config.needMemory) {
            dataProxy = std::make_shared<DataProxy<DataType>>(config.filePath, true, false);
        } else {
            dataProxy = std::make_shared<DataProxy<DataType>>(config.filePath, false, config.useTemporaryFile);
        }
        logging::getMemoryLogger().debug("DataManager for filename " + config.filePath + " created");
        logging::getMemoryLogger().debug("NeedMemory: " + std::to_string(config.needMemory));
        logging::getMemoryLogger().debug("UseTempFileParameter: " + std::to_string(config.useTemporaryFile));
    }

    std::shared_ptr<DataManager<DataType>> deepCopy() const {
        logging::getMemoryLogger().debug("DeepCopy is called.");
        std::shared_ptr<DataManager<DataType>> result =
            std::make_shared<DataManager<DataType>>(needMemory);
        result->locked = locked;
        result->dataProxy = std::make_shared<DataProxy<DataType>>(*dataProxy);
        logging::getMemoryLogger().debug("DeepCopy has finished.");
        return result;
    }

    void release() const {
        logging::getMemoryLogger().debug("Manager.release() is called.");
        assert(!locked);
        if (needMemory && !dataProxy->empty()) {
            dataProxy->releaseMemory();
        }
        logging::getMemoryLogger().debug("Manager.release() has finished.");
    }

    void load() const {
        logging::getMemoryLogger().debug("Manager.load() is called.");
        if (dataProxy->empty()) {
            dataProxy->loadData();
        }
        logging::getMemoryLogger().debug("Manager.load() has finished.");
    }

    void invalidate() {
        logging::getMemoryLogger().debug("Manager.invalidate() is called.");
        assert(!locked);
        dataProxy->invalidate();
        logging::getMemoryLogger().debug("Manager.invalidate() has finished.");
    }

    const DataType& getConstData() const {
        logging::getMemoryLogger().debug("Manager.getConstData() is called.");
        load();
        logging::getMemoryLogger().debug("Manager.getConstData() has finished.");
        return dataProxy->getConstData();
    }

    DataType& getData() {
        logging::getMemoryLogger().debug("Manager.getData() is called.");
        load();
        logging::getMemoryLogger().debug("Manager.getData() has finished.");
        return dataProxy->getData();
    }

    DataType* get() {
        logging::getMemoryLogger().debug("Manager.get() is called.");
        load();
        logging::getMemoryLogger().debug("Manager.get() has finished.");
        return dataProxy->get();
    }

    const DataType* get() const {
        logging::getMemoryLogger().debug("Manager.get() is called.");
        load();
        logging::getMemoryLogger().debug("Manager.get() has finished.");
        return dataProxy->get();
    }

    void lock() const {
        logging::getMemoryLogger().debug("Manager.lock() is called.");
        locked = true;
        logging::getMemoryLogger().debug("Manager.lock() has finished.");
    }
    void unlock() const {
        logging::getMemoryLogger().debug("Manager.unlock() is called.");
        locked = false;
        logging::getMemoryLogger().debug("Manager.unlock() has finished.");
    }

private:
    using DataProxyPtr=std::shared_ptr<DataProxy<DataType>>;

private:
    DataProxyPtr dataProxy;
    const bool needMemory;
    mutable bool locked = false;
};

using ConstDecisionManagerPtr=std::shared_ptr<const DataManager<Decision>>;
using DecisionManagerPtr=std::shared_ptr<DataManager<Decision>>;
using ConstActionManagerPtr=std::shared_ptr<const DataManager<Action>>;
using ActionManagerPtr=std::shared_ptr<DataManager<Action>>;
using ConstInputManagerPtr=std::shared_ptr<const DataManager<InputData>>;
using InputManagerPtr=std::shared_ptr<DataManager<InputData>>;
using ConstLinksManagerPtr=std::shared_ptr<const DataManager<InputLinks>>;
using LinksManagerPtr=std::shared_ptr<DataManager<InputLinks>>;
using ConstMarketManagerPtr=std::shared_ptr<const DataManager<MarketData>>;
using MarketManagerPtr=std::shared_ptr<DataManager<MarketData>>;
using IpoptIndexManagerPtr=std::shared_ptr<DataManager<backend::IpoptIndexMap>>;
using ConstIpoptIndexManagerPtr=std::shared_ptr<const DataManager<backend::IpoptIndexMap>>;
using DcpIndexManagerPtr=std::shared_ptr<DataManager<backend::DcpIndexMap>>;
using ConstDcpIndexManagerPtr=std::shared_ptr<const DataManager<backend::DcpIndexMap>>;
using LRIndexManagerPtr=std::shared_ptr<DataManager<backend::LagrangianRelaxationIndex>>;
using ConstLRIndexManagerPtr=std::shared_ptr<const DataManager<backend::LagrangianRelaxationIndex>>;

} // namespace sea
