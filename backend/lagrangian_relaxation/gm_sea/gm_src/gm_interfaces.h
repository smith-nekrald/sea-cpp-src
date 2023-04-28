#pragma once

#include <vector>
#include <map>
#include <string>
#include <memory>

#include <cppad/ipopt/solve.hpp>

namespace gm {

class IGradientOptimizer {
public:
    IGradientOptimizer(bool flagKeepStory, std::string aName)
        : keepStory(flagKeepStory)
        , name(aName) {}
    virtual std::vector<double> optimize(const std::vector<double>& init) const = 0;
    virtual ~IGradientOptimizer() {};
    std::map<std::string, std::vector<double>> getStory() const {
        return story;
    }
    std::string getName() const {
        return name;
    }
    bool isStoryKept() const {
        return keepStory;
    }
protected:
    bool keepStory;
    const std::string name;
    mutable std::map<std::string, std::vector<double>> story;
};

template<typename Value>
class IOrderZeroOracle {
public:
    virtual Value getValue(const std::vector<Value>& point) const = 0;
    virtual ~IOrderZeroOracle() {}
};

template<typename Value>
class IOrderOneOracle : public IOrderZeroOracle<Value> {
public:
    virtual std::vector<Value> getSubgradient(const std::vector<Value>& point) const = 0;
    virtual ~IOrderOneOracle() {}
};

using DoubleFunction=IOrderOneOracle<double>;
using CppADFunction=IOrderOneOracle<CppAD::AD<double>>;
using ThePoint=std::vector<double>;

template<typename ValueX, typename ValueY>
class BregmanDistance {
public:
    BregmanDistance(std::shared_ptr<const IOrderOneOracle<ValueX>> xProx,
            std::shared_ptr<const IOrderOneOracle<ValueY>> yProx)
        : xOracle(xProx)
        , yOracle(yProx) {
    }
    ValueY getDistance(const std::vector<ValueX>& xPoint,
            const std::vector<ValueY>& yPoint) const {
        auto size = xPoint.size();
        assert(xPoint.size() == yPoint.size());
        auto answer = yOracle->getValue(yPoint) - xOracle->getValue(xPoint);

        std::vector<ValueX> grad;
        grad = xOracle->getSubgradient(xPoint);

        ValueY scalar = 0;
        for (std::size_t idx = 0; idx < size; ++idx) {
            answer -= grad[idx] * (yPoint[idx] - xPoint[idx]);
            scalar += grad[idx] * (yPoint[idx] - xPoint[idx]);
        }

        // assert(answer + 1e-3 >= 0);
        return answer;
    }
private:
    std::shared_ptr<const IOrderOneOracle<ValueX>> xOracle;
    std::shared_ptr<const IOrderOneOracle<ValueY>> yOracle;
};

class IQDescriptor {
public:
    virtual std::vector<CppAD::AD<double>> makeConstraints(
            const std::vector<CppAD::AD<double>>& variables) const = 0;
    virtual void initConstraintsLR(
            std::vector<double>* glower, std::vector<double>* gupper) const = 0;
    virtual void initBoundsLR(
            std::vector<double>* vlower, std::vector<double>* vupper) const = 0;
    virtual ~IQDescriptor() {}
};

class IQProjector {
public:
    virtual std::vector<double> project (std::vector<double> point) const = 0;
    virtual ~IQProjector() {};

};

class IBregmanMapping {
public:
    virtual std::vector<double> compute(std::vector<double> point, double M) const = 0;
    virtual ~IBregmanMapping() {};
};

template<typename TypeX, typename TypeY>
class IPsiM {
public:
    virtual TypeY getValue(
            const std::vector<TypeX>& xPoint,
            const std::vector<TypeY>& yPoint,
            double M,
            std::vector<TypeX>* trgGrad=nullptr) const = 0;
    virtual ~IPsiM() {};
};

} // namespace gm
