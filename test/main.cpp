#define RWIN_FLAGS_OPERATORS
#include <iostream>
#include <vulkan/vulkan_core.h>
#include "rwin/DropCallbacks.h"
#include "rwin/rwin.h"
using namespace rwin;

struct WindowVulkanInfo
{
    vk::Fence renderFence;
    vk::Semaphore swapchainSemaphore;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    vk::SurfaceKHR surface;
    vk::Extent2D extent;
    vk::SwapchainKHR swapchain;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> imageViews;
    std::vector<vk::Semaphore> renderSemaphores;
};

vk::Instance instance;
vk::PhysicalDevice physicalDevice;
vk::Device device;
vk::Queue queue;
std::uint32_t queueFamilyIndex;
std::unordered_map<std::uint64_t, WindowVulkanInfo> windows;

void destroySwapchain(const std::uint64_t& windowId, WindowVulkanInfo& info)
{
    if (info.extent.width == 0 || info.extent.height == 0)
    {
        return;
    }
    for (auto renderSemaphore : info.renderSemaphores)
    {
        device.destroySemaphore(renderSemaphore);
    }
    for (auto view : info.imageViews)
    {
        device.destroyImageView(view);
    }
    info.renderSemaphores.clear();
    info.images.clear();
    info.imageViews.clear();

    device.destroySwapchainKHR(info.swapchain);

    info.extent = vk::Extent2D();
}

void createSwapchain(const std::uint64_t& windowId, WindowVulkanInfo& info)
{
    if (info.extent.width == 0 || info.extent.height == 0) return;
    auto caps = physicalDevice.getSurfaceCapabilitiesKHR(info.surface);
    auto formats = physicalDevice.getSurfaceFormatsKHR(info.surface);
    auto format = formats.front();
    vk::SwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.setImageFormat(format.format)
                       .setMinImageCount(caps.minImageCount).
                       setImageUsage(
                           vk::ImageUsageFlags{vk::ImageUsageFlagBits::eTransferDst} |
                           vk::ImageUsageFlagBits::eColorAttachment)
                       .setSurface(info.surface).
                       setPresentMode(vk::PresentModeKHR::eFifo)
                       .setImageColorSpace(format.colorSpace)
                       .setImageExtent(info.extent)
                       .setImageArrayLayers(1);
    info.swapchain = device.createSwapchainKHR(swapchainCreateInfo);
    info.images = device.getSwapchainImagesKHR(info.swapchain);
    vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers
    };
    for (const auto image : info.images)
    {
        info.renderSemaphores.push_back(device.createSemaphore({}));
        vk::ImageViewCreateInfo imageViewCreateInfo{};
        imageViewCreateInfo.setImage(image).setViewType(vk::ImageViewType::e2D).setFormat(format.format).
                            setSubresourceRange(subresourceRange);
        info.imageViews.push_back(device.createImageView(imageViewCreateInfo));
    }
}

void drawWindow(const std::uint64_t& windowId)
{
    auto& info = windows[windowId];
    auto size = info.extent;
    auto targetSize = physicalDevice.getSurfaceCapabilitiesKHR(info.surface).currentExtent;
    if (size != targetSize)
    {
        destroySwapchain(windowId, info);
        info.extent = targetSize;
        createSwapchain(windowId, info);
    }

    vk::detail::resultCheck(device.waitForFences(info.renderFence, true, std::numeric_limits<std::uint64_t>::max()),
                            "failed to wait for fences");
    device.resetFences(info.renderFence);
    device.resetCommandPool(info.commandPool);
    auto acquireResult = device.acquireNextImageKHR(info.swapchain, std::numeric_limits<std::uint64_t>::max(),
                                                    info.swapchainSemaphore, {});
    std::uint32_t swapchainImageIndex = acquireResult.value;

    auto& swapchainImage = info.images[swapchainImageIndex];
    auto cmd = info.commandBuffer;
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});
    vk::DependencyInfo depInfo{};
    vk::ImageMemoryBarrier2 imageMemoryBarrier{};
    vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers
    };
    imageMemoryBarrier
        .setSrcAccessMask(vk::AccessFlagBits2::eMemoryRead)
        .setDstAccessMask(vk::AccessFlags2{vk::AccessFlagBits2::eMemoryWrite} | vk::AccessFlagBits2::eMemoryRead)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eGeneral)
        .setImage(swapchainImage)
        .setSubresourceRange(subresourceRange);
    depInfo.setImageMemoryBarriers(imageMemoryBarrier);
    cmd.pipelineBarrier2(depInfo);
    cmd.clearColorImage(swapchainImage, vk::ImageLayout::eGeneral,
                        vk::ClearColorValue{}.setFloat32({1.0f, 1.0f, 1.0, 1.0f}), subresourceRange);
    imageMemoryBarrier.setOldLayout(imageMemoryBarrier.newLayout);
    imageMemoryBarrier.setNewLayout(vk::ImageLayout::ePresentSrcKHR);
    cmd.pipelineBarrier2(depInfo);
    cmd.end();

    auto& renderSemaphore = info.renderSemaphores[swapchainImageIndex];
    vk::CommandBufferSubmitInfo cmdSubmitInfo{cmd};
    vk::SemaphoreSubmitInfo renderSemaphoreInfo{renderSemaphore, 1, vk::PipelineStageFlagBits2::eAllGraphics};
    vk::SemaphoreSubmitInfo swapchainSemaphoreInfo{
        info.swapchainSemaphore, 1, vk::PipelineStageFlagBits2::eColorAttachmentOutput
    };
    vk::SubmitInfo2 submitInfo{};
    submitInfo.setCommandBufferInfos(cmdSubmitInfo).setSignalSemaphoreInfos(renderSemaphoreInfo).setWaitSemaphoreInfos(
        swapchainSemaphoreInfo);
    queue.submit2(submitInfo, info.renderFence);

    vk::PresentInfoKHR presentInfo{};

    presentInfo.setWaitSemaphores(renderSemaphore).setSwapchains(info.swapchain).setImageIndices(swapchainImageIndex);

    vk::detail::resultCheck(queue.presentKHR(presentInfo), "Failed to present");
}


void initVulkanWindow(const std::uint64_t& windowId)
{
    auto pool = device.createCommandPool({vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueFamilyIndex});
    auto cmd = device.allocateCommandBuffers({pool, vk::CommandBufferLevel::ePrimary, 1}).front();
    auto surface = createSurface(windowId, instance);
    auto fence = device.createFence({vk::FenceCreateFlagBits::eSignaled});
    auto semaphore = device.createSemaphore({});
    windows.emplace(windowId, WindowVulkanInfo{fence, semaphore, pool, cmd, surface, {0, 0}});
}

void destroyVulkanWindow(const std::uint64_t& windowId)
{
    auto& info = windows[windowId];
    device.waitIdle();
    destroySwapchain(windowId, info);
    device.destroyFence(info.renderFence);
    device.destroySemaphore(info.swapchainSemaphore);
    device.destroyCommandPool(info.commandPool);
    instance.destroySurfaceKHR(info.surface);
}

void initVulkan()
{
    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "rwin";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "rwin";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = vk::ApiVersion13;
    vk::InstanceCreateInfo instanceCreateInfo{};
    instanceCreateInfo.setPApplicationInfo(&appInfo);
    std::vector<const char*> extensions{};
    getRequiredExtensions(extensions);
    instanceCreateInfo.setPEnabledExtensionNames(extensions);
    instance = vk::createInstance(instanceCreateInfo);
    auto physicalDevices = instance.enumeratePhysicalDevices();
    physicalDevice = physicalDevices.front();

    for (auto queueFamily : physicalDevice.getQueueFamilyProperties())
    {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
        {
            break;
        }
        queueFamilyIndex++;
    }


    vk::DeviceQueueCreateInfo deviceQueueCreateInfo{};

    deviceQueueCreateInfo.queueCount = 1;

    deviceQueueCreateInfo.queueFamilyIndex = queueFamilyIndex;
    auto priority = 1.0f;
    deviceQueueCreateInfo.setQueuePriorities(priority);
    vk::DeviceCreateInfo deviceCreateInfo{};
    vk::PhysicalDeviceFeatures deviceFeatures{};
    vk::PhysicalDeviceSynchronization2Features sync2Features{};
    sync2Features.setSynchronization2(true);
    deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);
    deviceCreateInfo.setPEnabledExtensionNames({vk::KHRSwapchainExtensionName});
    deviceCreateInfo.setPNext(&sync2Features);

    deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
    device = physicalDevice.createDevice(deviceCreateInfo);

    queue = device.getQueue(queueFamilyIndex, 0);
}

void destroyVulkan()
{
    device.destroy();
    instance.destroy();
}

int main()
{
    initVulkan();
    const auto windowId = createWindow("Hello World", {1280, 720},
                                       WindowFlags::Visible | WindowFlags::Frameless | WindowFlags::DragAndDrop);
    initVulkanWindow(windowId);
    setWindowHitTestCallback(windowId, [windowId](const Vector2& point)
    {
        auto resizeBorderSize = 20;
        //std::cout << point.x << ", " << point.y << std::endl;
        auto windowSize = getWindowClientSize(windowId);

        if (point.y < 25)
        {
            return HitTestResult::DragArea;
        }

        if (point.x <= resizeBorderSize)
        {
            return HitTestResult::LeftResize;
        }

        if (point.x > windowSize.width - resizeBorderSize)
        {
            return HitTestResult::RightResize;
        }

        if (point.y >= windowSize.height - resizeBorderSize)
        {
            return HitTestResult::BottomResize;
        }

        return HitTestResult::None;
    });
    setWindowDropCallbacks(windowId, DropCallbacks{
                               .enter = [](const Vector2& pos, IDropContext* ctx)
                               {
                                   std::cout << "Drag Enter" << std::endl;
                                   return true;
                               },
                               .over = [](const Vector2& pos, IDropContext* ctx)
                               {
                                   return true;
                               },
                               .drop = [](const Vector2& pos, IDropContext* ctx)
                               {
                                   std::cout << "Drag Drop" << std::endl;
                                   if (ctx->HasFiles())
                                   {
                                       std::vector<std::filesystem::path> files{};
                                       ctx->GetFiles(files);
                                       for (auto &path : files)
                                       {
                                           std::cout << "file: " << path << std::endl;
                                       }
                                   }

                                   if (ctx->HasText())
                                   {
                                       std::vector<std::string> texts{};
                                       ctx->GetText(texts);
                                       for (auto &text : texts)
                                       {
                                           std::cout << "text: " << text << std::endl;
                                       }
                                   }
                               },
                               .leave = []()
                               {
                                   std::cout << "Drag Leave" << std::endl;
                               }
                           });
    std::vector<WindowEvent> events{};
    events.resize(64);
    auto quit = false;
    while (!quit)
    {
        pumpEvents();
        const auto eventsGotten = getEvents(events);
        for (auto i = 0; i < eventsGotten; i++)
        {
            const auto& event = events[i];
            if (event.info.windowId != windowId) continue;

            switch (event.info.type)
            {
            case WindowEventType::Close:
                quit = true;
                break;
            case WindowEventType::Key:
                {
                    if (event.key.key == InputKey::W && event.key.state == InputState::Pressed)
                    {
                        std::cout << "W Key Pressed" << std::endl;
                    }
                }
                break;
            default: ;
            }
        }
        drawWindow(windowId);
    }
    destroyVulkanWindow(windowId);
    destroyWindow(windowId);
    destroyVulkan();
    return 0;
}
