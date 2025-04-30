#pragma once

#include <string>
#include <vector>
#include <unordered_set>

namespace optimal_samples {

/**
 * @brief 表示一个样本的数据结构
 */
class Sample {
public:
    using ID = std::string;
    using Feature = std::string;
    using FeatureSet = std::unordered_set<Feature>;

    Sample(const ID& id, const FeatureSet& features)
        : id_(id), features_(features) {}

    // 获取样本ID
    const ID& getId() const { return id_; }

    // 获取样本特征集
    const FeatureSet& getFeatures() const { return features_; }

    // 检查样本是否包含特定特征
    bool hasFeature(const Feature& feature) const {
        return features_.find(feature) != features_.end();
    }

    // 添加特征
    void addFeature(const Feature& feature) {
        features_.insert(feature);
    }

    // 移除特征
    void removeFeature(const Feature& feature) {
        features_.erase(feature);
    }

    // 获取特征数量
    size_t getFeatureCount() const {
        return features_.size();
    }

private:
    ID id_;                    // 样本唯一标识符
    FeatureSet features_;      // 样本特征集合
};

} // namespace optimal_samples 