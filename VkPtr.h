/**
 * Copyright (C) 2022 Harry Nowland <harrynowl@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef VKPTR_H
#define VKPTR_H

#include <cstdint>
#include <functional>
#include <memory>

#ifndef VULKAN_H_
  // Include the Vulkan header if the user hasn't provided it
  #include <vulkan/vulkan.h>
#endif

#ifndef VKPTR_NAMESPACE
  // Use the namespace `vk`, however don't lock the user into
  // this choice
  #define VKPTR_NAMESPACE vk
#endif

// Allow the disabling of namespacing completely
#ifndef VKPTR_DISABLE_NAMESPACE
namespace VKPTR_NAMESPACE
{
#endif // VKPTR_DISABLE_NAMESPACE

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename Object>
using ObjectDeleter = std::function<void(Object, const VkAllocationCallbacks*)>;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename DeviceObject>
using InstanceObjectDeleter = std::function<void(VkInstance, DeviceObject, const VkAllocationCallbacks*)>;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename DeviceObject>
using DeviceObjectDeleter = std::function<void(VkDevice, DeviceObject, const VkAllocationCallbacks*)>;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename Pool, typename DeviceObject>
using PoolObjectDeleter = std::function<void(VkDevice, Pool, uint32_t, const DeviceObject*)>;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

using AnonymousDeleter = std::function<void()>;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

/**
 * Convenience wrapper to enable std::shared_ptr semantics for
 * Vulkan API objects such as VkFence and VkDevice
 *
 * \tparam T Vulkan object
 */
template<typename T>
class VkPtr
{
public:
  //! Underlying Vulkan object
  using VulkanObject = T;

  /**
   * Default constructor
   */
  VkPtr();

  /**
   * Allow nullptr based construction/assignment
   */
  VkPtr(std::nullptr_t);

  /**
   * Create a top level object wrapper with no current object, but
   * a valid deleter
   *
   * \param deleter     Object deleter (e.g. vkDestroyInstance, vkDestroyDevice)
   * \param callbacks   VkAllocationCallbacks
   */
  VkPtr(ObjectDeleter<T> deleter,
        const VkAllocationCallbacks* callbacks = nullptr);

  /**
   * Create a top level object wrapper
   *
   * \param object      Vulkan object to manage
   * \param deleter     Object deleter (e.g. vkDestroyInstance, vkDestroyDevice)
   * \param callbacks   VkAllocationCallbacks
   */
  VkPtr(VulkanObject object,
        ObjectDeleter<VulkanObject> deleter,
        const VkAllocationCallbacks* callbacks = nullptr);

  /**
   * Create an instance level object wrapper with no current object, but
   * a valid deleter
   *
   * \param instance    Vulkan instance wrapper
   * \param deleter     Object deleter (e.g. vkDestroySurfaceKHR)
   * \param callbacks   VkAllocationCallbacks
   */
  VkPtr(VkPtr<VkInstance> instance,
        InstanceObjectDeleter<VulkanObject> deleter,
        const VkAllocationCallbacks* callbacks = nullptr);

  /**
   * Create an instance level object wrapper
   *
   * \param instance    Vulkan instance wrapper
   * \param object      Vulkan object to manage
   * \param deleter     Object deleter (e.g. vkDestroySurfaceKHR)
   * \param callbacks   VkAllocationCallbacks
   */
  VkPtr(VkPtr<VkInstance> instance,
        VulkanObject object,
        InstanceObjectDeleter<VulkanObject> deleter,
        const VkAllocationCallbacks* callbacks = nullptr);

  /**
   * Create an device level object wrapper with no current object, but
   * a valid deleter
   *
   * \param device      Vulkan device wrapper
   * \param deleter     Object deleter (e.g. vkDestroyFence)
   * \param callbacks   VkAllocationCallbacks
   */
  VkPtr(VkPtr<VkDevice> device,
        DeviceObjectDeleter<VulkanObject> deleter,
        const VkAllocationCallbacks* callbacks = nullptr);

  /**
   * Create an device level object wrapper
   *
   * \param device      Vulkan device wrapper
   * \param object      Vulkan object to manage
   * \param deleter     Object deleter (e.g. vkDestroyFence)
   * \param callbacks   VkAllocationCallbacks
   */
  VkPtr(VkPtr<VkDevice> device,
        VulkanObject object,
        DeviceObjectDeleter<VulkanObject> deleter,
        const VkAllocationCallbacks* callbacks = nullptr);

  /**
   * Create an device level object wrapper which is issued from a pool,
   * with no current object, but a valid deleter
   *
   * \param device      Vulkan device wrapper
   * \param pool        Vulkan pool wrapper (VkCommandPool, VkDescriptorPool)
   * \param deleter     Object deleter (e.g. vkFreeCommandBuffers)
   */
  template<typename Pool>
  VkPtr(VkPtr<VkDevice> device,
        Pool pool,
        PoolObjectDeleter<typename Pool::VulkanObject, VulkanObject> deleter);

  /**
   * Create an device level object wrapper which is issued from a pool
   *
   * \param device      Vulkan device wrapper
   * \param pool        Vulkan pool wrapper (VkCommandPool, VkDescriptorPool)
   * \parma object      Vulkan object to manage
   * \param deleter     Object deleter (e.g. vkFreeCommandBuffers)
   */
  template<typename Pool>
  VkPtr(VkPtr<VkDevice> device,
        Pool pool,
        VulkanObject object,
        PoolObjectDeleter<typename Pool::VulkanObject, VulkanObject> deleter);

  /**
   * Destructor
   */
  ~VkPtr();

  /**
   * Get handle
   *
   * \return Handle, VK_NULL_HANDLE if object is not set
   */
  VulkanObject get() const;

  /**
   * Get handle
   *
   * \return Handle, VK_NULL_HANDLE if object is not set
   */
  VulkanObject operator*() const;

  /**
   * Get pointer to handle
   *
   * \return Pointer to handle, VK_NULL_HANDLE if object is not set
   */
  VulkanObject* pointer() const;

  /**
   * Release control of the handle
   *
   * @return Handle, no longer managed by this object
   */
  VulkanObject release();

private:
  struct Internal : public std::enable_shared_from_this<Internal>
  {
    /**
     * Allow default uninitialised construction
     */
    Internal()
      : handle(VK_NULL_HANDLE)
      , deleter()
    {}

    /**
     * Create a fully initialised internal object
     *
     * \param handle    Handle to delete (may be null)
     * \param deleter   Anonymous deleter for handle
     */
    Internal(VulkanObject handle, AnonymousDeleter deleter)
      : handle(handle)
      , deleter(std::move(deleter))
    {}

    /**
     * Destroy the Vulkan object using the anonymous deleter
     */
    ~Internal()
    {
      // Invoke if handle and deleter both valid
      if (deleter && handle) deleter();
    }

    //! Vulkan object
    VulkanObject handle;

    //! Delete function
    AnonymousDeleter deleter;
  };

  //! Use shared_ptr semantics to manage object lifetime
  std::shared_ptr<Internal> internal;
};

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr()
  : internal(std::make_shared<Internal>())
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(std::nullptr_t)
  : internal(std::make_shared<Internal>())
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(ObjectDeleter<T> deleter,
                const VkAllocationCallbacks* callbacks)
  : VkPtr(VK_NULL_HANDLE,
          std::move(deleter),
          callbacks)
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(T object,
                ObjectDeleter<T> deleter,
                const VkAllocationCallbacks* callbacks)
  : internal(std::make_shared<Internal>(object,
                                        [this, deleter, callbacks]
                                        {
                                          deleter(this->internal->handle, callbacks);
                                        }))
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(VkPtr<VkInstance> instance,
                InstanceObjectDeleter<T> deleter,
                const VkAllocationCallbacks* callbacks)
  : VkPtr(std::move(instance),
          VK_NULL_HANDLE,
          std::move(deleter),
          callbacks)
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(VkPtr<VkInstance> instance,
                T object,
                InstanceObjectDeleter<T> deleter,
                const VkAllocationCallbacks* callbacks)
  : internal(std::make_shared<Internal>(object,
                                        [this, instance, deleter, callbacks]
                                        {
                                          deleter(*instance, this->internal->handle, callbacks);
                                        }))
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(VkPtr<VkDevice> device,
                DeviceObjectDeleter<T> deleter,
                const VkAllocationCallbacks* callbacks)
  : VkPtr(std::move(device),
          VK_NULL_HANDLE,
          std::move(deleter),
          callbacks)
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::VkPtr(VkPtr<VkDevice> device,
                T object,
                DeviceObjectDeleter<T> deleter,
                const VkAllocationCallbacks* callbacks)
  : internal(std::make_shared<Internal>(object,
                                        [this, device, deleter, callbacks]
                                        {
                                          deleter(*device, this->internal->handle, callbacks);
                                        }))
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
template<typename Pool>
VkPtr<T>::VkPtr(VkPtr<VkDevice> device,
                Pool pool,
                PoolObjectDeleter<typename Pool::VulkanObject, T> deleter)
  : VkPtr(std::move(device),
          std::move(pool),
          VK_NULL_HANDLE,
          std::move(deleter))
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
template<typename Pool>
VkPtr<T>::VkPtr(VkPtr<VkDevice> device,
                Pool pool,
                T object,
                PoolObjectDeleter<typename Pool::VulkanObject, T> deleter)
  : internal(std::make_shared<Internal>(object,
                                        [this, device, pool, deleter]
                                        {
                                          deleter(*device, *pool, 1, &this->internal->handle);
                                        }))
{}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
VkPtr<T>::~VkPtr() = default;

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
T VkPtr<T>::get() const
{
  return internal->handle;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
T VkPtr<T>::operator*() const
{
  return internal->handle;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
T* VkPtr<T>::pointer() const
{
  return &internal->handle;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

template<typename T>
T VkPtr<T>::release()
{
  T handle = internal->handle;
  internal->handle = VK_NULL_HANDLE;
  return handle;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

#ifndef VKPTR_DISABLE_NAMESPACE
} // VKPTR_NAMESPACE
#endif // VKPTR_DISABLE_NAMESPACE

#endif // VKPTR_H
