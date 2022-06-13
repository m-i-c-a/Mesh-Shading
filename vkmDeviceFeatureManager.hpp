#ifndef VKM_DEVICE_FEATURE_MANAGER
#define VKM_DEVICE_FEATURE_MANAGER

#include <vector>

#include "vkmInit.hpp"

class vkmDeviceFeatureManager
{
private:
    std::vector<void*> featureStructs;
    const std::vector<SupportedDeviceFeature>& featureTypes;
    void* pNext = nullptr;
public:
    vkmDeviceFeatureManager(const std::vector<SupportedDeviceFeature>& requestedDeviceFeatures);
    ~vkmDeviceFeatureManager();

    void* getPNext() { return pNext; }
};

#endif // VKM_DEVICE_FEATURE_MANAGER