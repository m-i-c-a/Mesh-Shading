#include <vulkan/vulkan.h>

#include "vkmDeviceFeatureManager.hpp"
#include "Defines.hpp"

static void setPNext(void** pNext, void** prevStruct, void* nextStruct, SupportedDeviceFeature prevFeatureType)
{
    if ( *pNext == nullptr )
    {
        *pNext = nextStruct;
        *prevStruct = nextStruct;
    }
    else
    {
        switch (prevFeatureType)
        {
            case SupportedDeviceFeature::eSynchronization2:
            {
                static_cast<VkPhysicalDeviceSynchronization2FeaturesKHR*>(*prevStruct)->pNext = nextStruct;
                break;
            }
            case SupportedDeviceFeature::eDescriptorIndexing:
            {
                static_cast<VkPhysicalDeviceDescriptorIndexingFeatures*>(*prevStruct)->pNext = nextStruct;
                break;
            }
        };
    }
}

static void deleteFeatureStruct(void* featureStruct, SupportedDeviceFeature featureType)
{
    switch (featureType)
    {
        case SupportedDeviceFeature::eSynchronization2:
        {
            delete static_cast<VkPhysicalDeviceSynchronization2FeaturesKHR*>(featureStruct);
            break;
        }
        case SupportedDeviceFeature::eDescriptorIndexing:
        {
            delete static_cast<VkPhysicalDeviceDescriptorIndexingFeatures*>(featureStruct);
            break;
        }
    };
}

vkmDeviceFeatureManager::vkmDeviceFeatureManager(const std::vector<SupportedDeviceFeature>& requestedDeviceFeatures)
    : featureTypes { requestedDeviceFeatures }
{
   featureStructs.reserve(requestedDeviceFeatures.size());

   void* prevFeatureStruct = nullptr;
   SupportedDeviceFeature prevFeatureType = SupportedDeviceFeature::eInvalidFeature;

    for (const SupportedDeviceFeature feature : requestedDeviceFeatures)
    {
        switch (feature)
        {
            case SupportedDeviceFeature::eSynchronization2:
            {
                VkPhysicalDeviceSynchronization2FeaturesKHR* featureStruct = new VkPhysicalDeviceSynchronization2FeaturesKHR();
                featureStruct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES_KHR;
                featureStruct->synchronization2 = VK_TRUE;

                featureStructs.push_back( featureStruct );

                setPNext(&pNext, &prevFeatureStruct, featureStruct, prevFeatureType);
                prevFeatureType = SupportedDeviceFeature::eSynchronization2;

                break;
            }
            case SupportedDeviceFeature::eDescriptorIndexing:
            {
                VkPhysicalDeviceDescriptorIndexingFeatures* featureStruct = new VkPhysicalDeviceDescriptorIndexingFeatures();
                featureStruct->sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
                featureStruct->shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
                featureStruct->runtimeDescriptorArray = VK_TRUE;
                featureStruct->descriptorBindingPartiallyBound = VK_TRUE;
                featureStruct->descriptorBindingVariableDescriptorCount = VK_TRUE;

                featureStructs.push_back( featureStruct );

                setPNext(&pNext, &prevFeatureStruct, featureStruct, prevFeatureType);
                prevFeatureType = SupportedDeviceFeature::eDescriptorIndexing;

                break;
            }
            case SupportedDeviceFeature::eInvalidFeature:
            {
                EXIT("Invalid Feature cannot be a requested device feature\n");
            }
        };
    }
}

vkmDeviceFeatureManager::~vkmDeviceFeatureManager()
{
    for (size_t i = 0; i < featureStructs.size(); ++i)
    {
        deleteFeatureStruct(featureStructs[i], featureTypes[i]);
    }
}