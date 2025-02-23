// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "kompute/Core.hpp"
#include <fmt/format.h>
#include <string>

namespace kp {

/**
 * Structured data used in GPU operations.
 *
 * Tensors are the base building block in Kompute to perform operations across
 * GPUs. Each tensor would have a respective Vulkan memory and buffer, which
 * would be used to store their respective data. The tensors can be used for GPU
 * data storage or transfer.
 */
class Tensor
{
  public:
    /**
     * Type for tensors created: Device allows memory to be transferred from
     * staging buffers. Staging are host memory visible. Storage are device
     * visible but are not set up to transfer or receive data (only for shader
     * storage).
     */
    enum class TensorTypes
    {
        eDevice = 0,  ///< Type is device memory, source and destination
        eHost = 1,    ///< Type is host memory, source and destination
        eStorage = 2, ///< Type is Device memory (only)
    };
    enum class TensorDataTypes
    {
        eBool = 0,
        eInt = 1,
        eUnsignedInt = 2,
        eFloat = 3,
        eDouble = 4,
    };

    /**
     *  Constructor with data provided which would be used to create the
     * respective vulkan buffer and memory.
     *
     *  @param physicalDevice The physical device to use to fetch properties
     *  @param device The device to use to create the buffer and memory from
     *  @param data Non-zero-sized vector of data that will be used by the
     * tensor
     *  @param tensorTypes Type for the tensor which is of type TensorTypes
     */
    Tensor(std::shared_ptr<vk::PhysicalDevice> physicalDevice,
           std::shared_ptr<vk::Device> device,
           void* data,
           uint32_t elementTotalCount,
           uint32_t elementMemorySize,
           const TensorDataTypes& dataType,
           const TensorTypes& tensorType = TensorTypes::eDevice);

    /**
     * Destructor which is in charge of freeing vulkan resources unless they
     * have been provided externally.
     */
    virtual ~Tensor();

    /**
     * Function to trigger reinitialisation of the tensor buffer and memory with
     * new data as well as new potential device type.
     *
     * @param data Vector of data to use to initialise vector from
     * @param tensorType The type to use for the tensor
     */
    void rebuild(void* data,
                 uint32_t elementTotalCount,
                 uint32_t elementMemorySize);

    /**
     * Destroys and frees the GPU resources which include the buffer and memory.
     */
    void destroy();

    /**
     * Check whether tensor is initialized based on the created gpu resources.
     *
     * @returns Boolean stating whether tensor is initialized
     */
    bool isInit();

    /**
     * Retrieve the tensor type of the Tensor
     *
     * @return Tensor type of tensor
     */
    TensorTypes tensorType();

    /**
     * Records a copy from the memory of the tensor provided to the current
     * thensor. This is intended to pass memory into a processing, to perform
     * a staging buffer transfer, or to gather output (between others).
     *
     * @param commandBuffer Vulkan Command Buffer to record the commands into
     * @param copyFromTensor Tensor to copy the data from
     */
    void recordCopyFrom(const vk::CommandBuffer& commandBuffer,
                        std::shared_ptr<Tensor> copyFromTensor);

    /**
     * Records a copy from the internal staging memory to the device memory
     * using an optional barrier to wait for the operation. This function would
     * only be relevant for kp::Tensors of type eDevice.
     *
     * @param commandBuffer Vulkan Command Buffer to record the commands into
     */
    void recordCopyFromStagingToDevice(const vk::CommandBuffer& commandBuffer);

    /**
     * Records a copy from the internal device memory to the staging memory
     * using an optional barrier to wait for the operation. This function would
     * only be relevant for kp::Tensors of type eDevice.
     *
     * @param commandBuffer Vulkan Command Buffer to record the commands into
     */
    void recordCopyFromDeviceToStaging(const vk::CommandBuffer& commandBuffer);

    /**
     * Records the buffer memory barrier into the primary buffer and command
     * buffer which ensures that relevant data transfers are carried out
     * correctly.
     *
     * @param commandBuffer Vulkan Command Buffer to record the commands into
     * @param srcAccessMask Access flags for source access mask
     * @param dstAccessMask Access flags for destination access mask
     * @param scrStageMask Pipeline stage flags for source stage mask
     * @param dstStageMask Pipeline stage flags for destination stage mask
     */
    void recordPrimaryBufferMemoryBarrier(
      const vk::CommandBuffer& commandBuffer,
      vk::AccessFlagBits srcAccessMask,
      vk::AccessFlagBits dstAccessMask,
      vk::PipelineStageFlagBits srcStageMask,
      vk::PipelineStageFlagBits dstStageMask);
    /**
     * Records the buffer memory barrier into the staging buffer and command
     * buffer which ensures that relevant data transfers are carried out
     * correctly.
     *
     * @param commandBuffer Vulkan Command Buffer to record the commands into
     * @param srcAccessMask Access flags for source access mask
     * @param dstAccessMask Access flags for destination access mask
     * @param scrStageMask Pipeline stage flags for source stage mask
     * @param dstStageMask Pipeline stage flags for destination stage mask
     */
    void recordStagingBufferMemoryBarrier(
      const vk::CommandBuffer& commandBuffer,
      vk::AccessFlagBits srcAccessMask,
      vk::AccessFlagBits dstAccessMask,
      vk::PipelineStageFlagBits srcStageMask,
      vk::PipelineStageFlagBits dstStageMask);

    /**
     * Constructs a vulkan descriptor buffer info which can be used to specify
     * and reference the underlying buffer component of the tensor without
     * exposing it.
     *
     * @return Descriptor buffer info with own buffer
     */
    vk::DescriptorBufferInfo constructDescriptorBufferInfo();

    /**
     * Returns the size/magnitude of the Tensor, which will be the total number
     * of elements across all dimensions
     *
     * @return Unsigned integer representing the total number of elements
     */
    uint32_t size();

    /**
     * Returns the total size of a single element of the respective data type
     * that this tensor holds.
     *
     * @return Unsigned integer representing the memory of a single element of
     * the respective data type.
     */
    uint32_t dataTypeMemorySize();

    /**
     * Returns the total memory size of the data contained by the Tensor object
     * which would equate to (this->size() * this->dataTypeMemorySize())
     *
     * @return Unsigned integer representing the memory of a single element of
     * the respective data type.
     */
    uint32_t memorySize();

    /**
     * Retrieve the data type of the tensor (host, device, storage)
     *
     * @return Data type of tensor of type kp::Tensor::TensorDataTypes
     */
    TensorDataTypes dataType();

    /**
     * Retrieve the raw data via the pointer to the memory that contains the raw
     * memory of this current tensor. This tensor gets changed to a nullptr when
     * the Tensor is removed.
     *
     * @return Pointer to raw memory containing raw bytes data of Tensor.
     */
    void* rawData();

    /**
     * Sets / resets the data of the tensor which is directly done on the GPU
     * host visible memory available by the tensor.
     */
    void setRawData(const void* data);

    /**
     * Template to return the pointer data converted by specific type, which
     * would be any of the supported types including float, double, int32,
     * uint32 and bool.
     *
     * @return Pointer to raw memory containing raw bytes data of Tensor.
     */
    template<typename T>
    T* data()
    {
        return (T*)this->mRawData;
    }

    /**
     * Template to get the data of the current tensor as a vector of specific
     * type, which would be any of the supported types including float, double,
     * int32, uint32 and bool.
     *
     * @return Vector of type provided by template.
     */
    template<typename T>
    std::vector<T> vector()
    {
        return { (T*)this->mRawData, ((T*)this->mRawData) + this->size() };
    }

  protected:
    // -------------- ALWAYS OWNED RESOURCES
    TensorTypes mTensorType;
    TensorDataTypes mDataType;
    uint32_t mSize;
    uint32_t mDataTypeMemorySize;
    void* mRawData;

  private:
    // -------------- NEVER OWNED RESOURCES
    std::shared_ptr<vk::PhysicalDevice> mPhysicalDevice;
    std::shared_ptr<vk::Device> mDevice;

    // -------------- OPTIONALLY OWNED RESOURCES
    std::shared_ptr<vk::Buffer> mPrimaryBuffer;
    bool mFreePrimaryBuffer = false;
    std::shared_ptr<vk::Buffer> mStagingBuffer;
    bool mFreeStagingBuffer = false;
    std::shared_ptr<vk::DeviceMemory> mPrimaryMemory;
    bool mFreePrimaryMemory = false;
    std::shared_ptr<vk::DeviceMemory> mStagingMemory;
    bool mFreeStagingMemory = false;

    void allocateMemoryCreateGPUResources(); // Creates the vulkan buffer
    void createBuffer(std::shared_ptr<vk::Buffer> buffer,
                      vk::BufferUsageFlags bufferUsageFlags);
    void allocateBindMemory(std::shared_ptr<vk::Buffer> buffer,
                            std::shared_ptr<vk::DeviceMemory> memory,
                            vk::MemoryPropertyFlags memoryPropertyFlags);
    void recordCopyBuffer(const vk::CommandBuffer& commandBuffer,
                          std::shared_ptr<vk::Buffer> bufferFrom,
                          std::shared_ptr<vk::Buffer> bufferTo,
                          vk::DeviceSize bufferSize,
                          vk::BufferCopy copyRegion);
    void recordBufferMemoryBarrier(const vk::CommandBuffer& commandBuffer,
                                   const vk::Buffer& buffer,
                                   vk::AccessFlagBits srcAccessMask,
                                   vk::AccessFlagBits dstAccessMask,
                                   vk::PipelineStageFlagBits srcStageMask,
                                   vk::PipelineStageFlagBits dstStageMask);

    // Private util functions
    vk::BufferUsageFlags getPrimaryBufferUsageFlags();
    vk::MemoryPropertyFlags getPrimaryMemoryPropertyFlags();
    vk::BufferUsageFlags getStagingBufferUsageFlags();
    vk::MemoryPropertyFlags getStagingMemoryPropertyFlags();

    void mapRawData();
    void unmapRawData();
};

template<typename T>
class TensorT : public Tensor
{

  public:
    TensorT(std::shared_ptr<vk::PhysicalDevice> physicalDevice,
            std::shared_ptr<vk::Device> device,
            const std::vector<T>& data,
            const TensorTypes& tensorType = TensorTypes::eDevice)
      : Tensor(physicalDevice,
               device,
               (void*)data.data(),
               data.size(),
               sizeof(T),
               this->dataType(),
               tensorType)
    {
        KP_LOG_DEBUG("Kompute TensorT constructor with data size {}",
                     data.size());
    }

    ~TensorT() { KP_LOG_DEBUG("Kompute TensorT destructor"); }

    T* data() { return (T*)this->mRawData; }

    std::vector<T> vector()
    {
        return { (T*)this->mRawData, ((T*)this->mRawData) + this->size() };
    }

    T& operator[](int index) { return *(((T*)this->mRawData) + index); }

    void setData(const std::vector<T>& data)
    {

        KP_LOG_DEBUG("Kompute TensorT setting data with data size {}",
                     data.size());

        if (data.size() != this->mSize) {
            throw std::runtime_error(
              "Kompute TensorT Cannot set data of different sizes");
        }

        Tensor::setRawData(data.data());
    }

    TensorDataTypes dataType();
};

} // End namespace kp

/**
 * fmt fromater for kp::Tensor::TensorDataTypes.
 */
template<>
struct fmt::formatter<kp::Tensor::TensorDataTypes> : formatter<std::string>
{
    template<typename FormatContext>
    auto format(kp::Tensor::TensorDataTypes dt, FormatContext& ctx)
    {
        std::string name = "unknown";
        switch (dt) {
            case kp::Tensor::TensorDataTypes::eBool:
                name = "eBool";
                break;
            case kp::Tensor::TensorDataTypes::eDouble:
                name = "eDouble";
                break;
            case kp::Tensor::TensorDataTypes::eFloat:
                name = "eFloat";
                break;
            case kp::Tensor::TensorDataTypes::eInt:
                name = "eInt";
                break;
            case kp::Tensor::TensorDataTypes::eUnsignedInt:
                name = "eUnsignedInt";
                break;
        }
        return formatter<std::string>::format(name, ctx);
    }
};

/**
 * fmt fromater for kp::Tensor::TensorTypes.
 */
template<>
struct fmt::formatter<kp::Tensor::TensorTypes> : formatter<std::string>
{
    template<typename FormatContext>
    auto format(kp::Tensor::TensorTypes dt, FormatContext& ctx)
    {
        std::string name = "unknown";
        switch (dt) {
            case kp::Tensor::TensorTypes::eDevice:
                name = "eDevice";
                break;
            case kp::Tensor::TensorTypes::eHost:
                name = "eHost";
                break;
            case kp::Tensor::TensorTypes::eStorage:
                name = "eStorage";
                break;
        }
        return formatter<std::string>::format(name, ctx);
    }
};
