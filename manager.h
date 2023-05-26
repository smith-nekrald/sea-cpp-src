/**
 * @file manager.h
 * @author Aliaksandr Nekrashevich (aliaksandr.nekrashevich@queensu.ca)
 * @brief Implements DataProxy and DataManager. DataProxy is an entity for manual RAM memory
 * control by dumping structures when RAM-instensive computations are ongoing.
 * @copyright (c) Smith School of Business, 2023
 */
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

/**
 * @brief Structure for manual memory consumption manager.
 * Allows to dump structures to file when big RAM amounts are consumed.
 *
 * @tparam DataType Type of the incapsulated object.
 */
template<typename DataType>
class DataProxy {
public:
    /**
     * @brief Proxy Constructor.
     *
     * @param aFilePath Permanent storate file path, if relevant. Empty string otherwise.
     * @param leaveEmpty Whether to load the data or not.
     * @param useTemporaryFileVal If true, temporary file is used for dumping-undumping.
     */
    DataProxy(std::string aFilePath, bool leaveEmpty = false, bool useTemporaryFileVal = true)
            : filePath(aFilePath)
            , useTemporaryFile(useTemporaryFileVal)
            , isDumped(!useTemporaryFileVal)
            , isValid(true) {
        if (leaveEmpty) {
            assert(!useTemporaryFile);
            return;
        }
        if (!useTemporaryFile) {
            loadData();
        } else {
            data = std::make_unique<DataType>();
        }
        isEmpty = false;
    }
    /**
     * @brief Copy constructor.
     *
     * @param proxy The DataProxy to copy.
     */
    DataProxy(const DataProxy& proxy)
            : useTemporaryFile(proxy.useTemporaryFile) {
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
    }
    /**
     * @brief Getter for the editable reference to the encapsulated Data object.
     *
     * @return Editable reference to the encapculated object.
     */
    DataType& getData() {
        assert(isValid && !isEmpty);
        return *data;
    }
    /**
     * @brief Getter for the editable pointer to the encapsulated Data object.
     *
     * @return Editable pointer to the encapculated object.
     */
    DataType* get() {
        assert(isValid && !isEmpty);
        return data.get();
    }
    /**
     * @brief Getter for the constant reference to the encapsulated Data object.
     *
     * @return Constant reference to the encapculated object.
     */
    const DataType& getConstData() const {
        assert(isValid && !isEmpty);
        return *data;
    }
    /**
     * @brief Method to release allocated RAM.
     */
    void releaseMemory() {
        assert(isValid && !isEmpty);
        if (!isDumped) {
            dumpData();
        }
        assert(isDumped);
        data.reset();
        isEmpty = true;
    }
    /**
     * @brief Dumps data to file.
     */
    void dumpData() {
        assert(isValid && !isEmpty && !isDumped);
        if (useTemporaryFile) {
            writeToFile(filePath, *data);
            isDumped = true;
        }
        assert(isDumped);
    }
    /**
     * @brief Loads data from file.
     */
    void loadData() {
        assert(isValid && isDumped && isEmpty);
        data = std::make_unique<DataType>();
        DataType* rawPointer = data.get();
        loadFromFile(filePath, rawPointer);
        if (useTemporaryFile) {
            removeTemporaryFile();
            isDumped = false;
        }
        isEmpty = false;
    }
    /**
     * @brief Removes temporary file.
     */
    void removeTemporaryFile() {
        assert(useTemporaryFile);
        if (std::experimental::filesystem::is_regular_file(filePath)) {
            std::experimental::filesystem::remove(filePath);
        }
    }
    /**
     * @brief Resets to non-initialized state. However, preserves type and permanent location.
     * I.e. InputData or MarketData would load the same, however those with temporary file
     * dumping reset completely.
     */
    void invalidate() {
        if (useTemporaryFile) {
            removeTemporaryFile();
        }
        data.reset();
        isValid = false;
    }
    /**
     * @brief Check if the data pointer is set somewhere.
     *
     * @return True if pointer is unset, False otherwise.
     */
    bool empty() {
        assert(isValid);
        return isEmpty;
    }
    /**
     * @brief Check if dumped.
     *
     * @return True if dumped, False otherwise.
     */
    bool dumped() {
        assert(isValid);
        return isDumped;
    }
    /**
     * @brief Check if the object is valid.
     *
     * @return True if the object is valid, False otherwise.
     */
    bool valid() {
        return isValid;
    }
    /**
     * @brief Destructor the Data Proxy object.
     * Temporary file is removed only if the dump location is non-permanent.
     */
    virtual ~DataProxy() {
        if (useTemporaryFile) {
            removeTemporaryFile();
        }
    }

private:
    /// @brief The path to dumped file.
    std::string filePath;
    /// @brief If true, temporary storage file is used.
    const bool useTemporaryFile;

    /// @brief The pointer to the encapsulated data.
    std::unique_ptr<DataType> data;
    /// @brief Whether the object is empty.
    bool isEmpty = true;
    /// @brief Whether the object is dumped.
    bool isDumped;
    /// @brief Whether the object is valid.
    bool isValid = true;
};

/**
 * @brief Configures data Manager.
 */
struct ManagerConfig {
    /// @brief If true, memory tricks are applied. Otherwise, not applied.
    bool needMemory;
    /// @brief The path to permanent file with stored object.
    std::string filePath;
    /// @brief If to use temporary file for dumping-undumping object.
    bool useTemporaryFile;
};

/**
 * @brief Data Manager is a wrapper around Data Proxy. The extended functionality is as follows.
 * If needMemory flag is set to false, releases and loads are not performed. The data is always
 * loaded and ready to use. Another extension corresponds to locking. If locked, load-release are
 * not performed. If unlocked, everything works as usual.
 *
 * @tparam DataType The type of encapsulated object.
 */
template<typename DataType>
class DataManager {

public:
    /**
     * @brief Flag-based constructor for a new Data Manager. Data proxy is set to nullptr.
     *
     * @param memoryNeed If true, memory management tricks are performed in the same style as
     * data proxy. If false, memory management tricks are ignored.
     */
    DataManager(bool memoryNeed = false)
            : needMemory(memoryNeed) {
        dataProxy = nullptr;
    }
    /**
     * @brief Construct a new Data Manager object from config.
     *
     * @param config The Configuration for Data Manager.
     */
    DataManager(const ManagerConfig& config)
            : needMemory(config.needMemory) {
        if (!config.useTemporaryFile && config.needMemory) {
            dataProxy = std::make_shared<DataProxy<DataType>>(config.filePath, true, false);
        } else {
            dataProxy = std::make_shared<DataProxy<DataType>>(
                config.filePath, false, config.useTemporaryFile);
        }
    }
    /**
     * @brief Deep copy method. Creates new dataProxy by copy constructor
     * and replicates the lock state.
     *
     * @return A pointer to a new data manager.
     */
    std::shared_ptr<DataManager<DataType>> deepCopy() const {
        std::shared_ptr<DataManager<DataType>> result =
            std::make_shared<DataManager<DataType>>(needMemory);
        result->locked = locked;
        result->dataProxy = std::make_shared<DataProxy<DataType>>(*dataProxy);
        return result;
    }
    /**
     * @brief Method for non-locked object. If memory management is turned on, releases
     * memory in data proxy.
     */
    void release() const {
        assert(!locked);
        if (needMemory && !dataProxy->empty()) {
            dataProxy->releaseMemory();
        }
    }
    /**
     * @brief Loads data proxy in case the proxy is released.
     */
    void load() const {
        if (dataProxy->empty()) {
            dataProxy->loadData();
        }
    }
    /**
     * @brief Resets to the non-initialized state.
     */
    void invalidate() {
        assert(!locked);
        dataProxy->invalidate();
    }
    /**
     * @brief Constant reference getter for an incapsulated object.
     *
     * @return Constant reference to the object.
     */
    const DataType& getConstData() const {
        load();
        return dataProxy->getConstData();
    }
    /**
     * @brief Editable reference getter for an incapculated object.
     *
     * @return Editable reference to the object.
     */
    DataType& getData() {
        load();
        return dataProxy->getData();
    }
    /**
     * @brief Editable pointer getter for an incapsulated object.
     *
     * @return Editable pointer to the object.
     */
    DataType* get() {
        load();
        return dataProxy->get();
    }
    /**
     * @brief  Constant pointer getter for an incapsulated object.
     *
     * @return Constant pointer to the object.
     */
    const DataType* get() const {
        load();
        return dataProxy->get();
    }
    /**
     * @brief Locks the manager.
     */
    void lock() const {
        locked = true;
    }
    /**
     * @brief Unlocks the manager.
     */
    void unlock() const {
        locked = false;
    }

private:
    /// @brief Simplified type name for a shared pointer to the data proxy.
    using DataProxyPtr=std::shared_ptr<DataProxy<DataType>>;

private:
    /// @brief Proxy with encapsulated object.
    DataProxyPtr dataProxy;
    /// @brief If memory management is needed.
    const bool needMemory;
    /// @brief Flag to account for the locked-unlocked state.
    mutable bool locked = false;
};

/// @brief Simpified type name for shared pointer to the constant Decision Manager.
using ConstDecisionManagerPtr=std::shared_ptr<const DataManager<Decision>>;
/// @brief Simplified type name for shared pointer to Decision Manager.
using DecisionManagerPtr=std::shared_ptr<DataManager<Decision>>;

/// @brief Simplified type name for shared pointer to constant Action Manager.
using ConstActionManagerPtr=std::shared_ptr<const DataManager<Action>>;
/// @brief Simplified type name for shared pointer to Action Manager.
using ActionManagerPtr=std::shared_ptr<DataManager<Action>>;

/// @brief Simplified type name for shared pointer to constant Input Data Manager.
using ConstInputManagerPtr=std::shared_ptr<const DataManager<InputData>>;
/// @brief Simplified type name for shared pointer to Input Data Manager.
using InputManagerPtr=std::shared_ptr<DataManager<InputData>>;

/// @brief Simplified type name for shared pointer to constant Input Links Manager.
using ConstLinksManagerPtr=std::shared_ptr<const DataManager<InputLinks>>;
/// @brief Simplified type name for shared pointer to Input Links Manager.
using LinksManagerPtr=std::shared_ptr<DataManager<InputLinks>>;

/// @brief Simplified type name for shared pointer to constant Market Data Manager.
using ConstMarketManagerPtr=std::shared_ptr<const DataManager<MarketData>>;
/// @brief Simplified type name for shared pointer to Market Data Manager.
using MarketManagerPtr=std::shared_ptr<DataManager<MarketData>>;

/// @brief Simplified type name for shared pointer to constant IpoptIndexMap Manager.
using ConstIpoptIndexManagerPtr=std::shared_ptr<const DataManager<backend::IpoptIndexMap>>;
/// @brief Simplified type name for shared pointer to constant IpoptIndexMap Manager.
using IpoptIndexManagerPtr=std::shared_ptr<DataManager<backend::IpoptIndexMap>>;

/// @brief Simplified type name for shared pointer to constant DcpIndexMap Manager.
using ConstDcpIndexManagerPtr=std::shared_ptr<const DataManager<backend::DcpIndexMap>>;
/// @brief Simplified type name for shared pointer to DcpIndexMap Manager.
using DcpIndexManagerPtr=std::shared_ptr<DataManager<backend::DcpIndexMap>>;

/// @brief Simplified type name for shared pointer to constant LR Index Manager.
using ConstLRIndexManagerPtr=std::shared_ptr<
    const DataManager<backend::LagrangianRelaxationIndex>>;
/// @brief Simplified type name for shared pointer to LR Index Manager.
using LRIndexManagerPtr=std::shared_ptr<DataManager<backend::LagrangianRelaxationIndex>>;

} // namespace sea
