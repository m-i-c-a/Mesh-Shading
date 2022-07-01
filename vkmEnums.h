#ifndef VKM_ENUMS_HPP
#define VKM_ENUMS_HPP

enum
{
    QUEUE_GRAPHICS = 0,
    QUEUE_COUNT
};

enum
{
    COMMAND_POOL_DEFAULT = 0,
    COMMAND_POOL_COUNT
};

enum
{
    COMMAND_BUFFER_DEFAULT = 0,
    COMMAND_BUFFER_COUNT
};

enum
{
    SEMAPHORE_COUNT
};

enum
{
    FENCE_IMAGE_ACQUIRE = 0,
    FENCE_COUNT
};

enum
{
    DESCRIPTOR_POOL_IMGUI = 0,
    DESCRIPTOR_POOL_DEFAULT = 1,
    DESCRIPTOR_POOL_COUNT
};

enum
{
    DESCRIPTOR_SET_LAYOUT_DEFAULT_0 = 0,
    DESCRIPTOR_SET_LAYOUT_DEFAULT_1 = 1,
    DESCRIPTOR_SET_LAYOUT_COUNT
};

enum
{
    DESCRIPTOR_SET_FRAME    = 0,
    DESCRIPTOR_SET_MATERIAL = 1,
    DESCRIPTOR_SET_COUNT
};

enum
{
    BUFFER_STAGING       = 0,
    BUFFER_GEOMETRY_SSBO = 1,
    BUFFER_OBJECT_INDEX  = 2,
    BUFFER_OBJECT_VERTEX = 3,
    BUFFER_PER_FRAME_UBO = 4,
    BUFFER_LIGHT_UBO     = 5,
    BUFFER_MATERIAL_UBO  = 6,
    BUFFER_COUNT
};

enum
{
    ATTACHMENT_DEPTH = 0,
    ATTACHMENT_COUNT
};

enum
{
    IMAGE_COUNT
};

enum
{
    IMAGE_VIEW_DEPTH = 0,
    IMAGE_VIEW_COUNT
};

#endif // VKM_ENUMS_HPP