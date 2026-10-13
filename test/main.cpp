#define RWIN_FLAGS_OPERATORS
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vulkan/vulkan_core.h>
#include <imgui_impl_vulkan.h>
#include "rwin/rwin.h"
#include "ImGuiRwin.h"
using namespace rwin;

static void checkResult(const vk::Result result, const char* message)
{
    if (result != vk::Result::eSuccess)
    {
        throw std::runtime_error(message);
    }
}

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
    vk::Format format;
    bool stale = false;
};

vk::Instance instance;
vk::PhysicalDevice physicalDevice;
vk::Device device;
vk::Queue queue;
std::uint32_t queueFamilyIndex;
std::unordered_map<std::uint64_t, WindowVulkanInfo> windows;
vk::DescriptorPool imguiPool;
bool imguiVulkanReady = false;

void destroySwapchain(const std::uint64_t& windowId, WindowVulkanInfo& info)
{
    if (info.extent.width == 0 || info.extent.height == 0)
    {
        return;
    }
    device.waitIdle();
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
    // ImGui's colors are already sRGB encoded, an sRGB format would encode them a second time
    for (const auto& candidate : formats)
    {
        if (candidate.format == vk::Format::eB8G8R8A8Unorm || candidate.format == vk::Format::eR8G8B8A8Unorm)
        {
            format = candidate;
            break;
        }
    }
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
    info.format = format.format;
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

void initImGuiVulkan(const WindowVulkanInfo& info)
{
    const vk::DescriptorPoolSize poolSize{vk::DescriptorType::eCombinedImageSampler, 16};
    imguiPool = device.createDescriptorPool(
        vk::DescriptorPoolCreateInfo{vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 16, poolSize});

    static VkFormat colorFormat;
    colorFormat = static_cast<VkFormat>(info.format);

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = static_cast<VkInstance>(instance);
    initInfo.PhysicalDevice = static_cast<VkPhysicalDevice>(physicalDevice);
    initInfo.Device = static_cast<VkDevice>(device);
    initInfo.QueueFamily = queueFamilyIndex;
    initInfo.Queue = static_cast<VkQueue>(queue);
    initInfo.DescriptorPool = static_cast<VkDescriptorPool>(imguiPool);
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = std::max<std::uint32_t>(2, static_cast<std::uint32_t>(info.images.size()));
    initInfo.UseDynamicRendering = true;
    initInfo.PipelineRenderingCreateInfo = VkPipelineRenderingCreateInfoKHR{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &colorFormat,
    };
    ImGui_ImplVulkan_Init(&initInfo);
}

void drawWindow(const std::uint64_t& windowId)
{
    const auto clientExtent = getWindowClientSize(windowId);
    if (clientExtent.width == 0 || clientExtent.height == 0)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        return;
    }

    auto& info = windows[windowId];
    const vk::Extent2D targetSize{clientExtent.width, clientExtent.height};

    checkResult(device.waitForFences(info.renderFence, true, std::numeric_limits<std::uint64_t>::max()),
                "failed to wait for fences");

    if (info.extent != targetSize || info.stale)
    {
        destroySwapchain(windowId, info);
        info.extent = targetSize;
        createSwapchain(windowId, info);
        info.stale = false;
    }

    if (!imguiVulkanReady)
    {
        initImGuiVulkan(info);
        imguiVulkanReady = true;
    }

    static auto lastFrame = std::chrono::steady_clock::now();
    const auto now = std::chrono::steady_clock::now();
    const auto deltaTime = std::chrono::duration<float>(now - lastFrame).count();
    lastFrame = now;

    ImGui_ImplVulkan_NewFrame();
    imguirwin::newFrame(deltaTime);
    ImGui::NewFrame();
    imguirwin::drawUi();
    ImGui::Render();
    imguirwin::endFrame();

    std::uint32_t swapchainImageIndex = 0;
    try
    {
        const auto acquireResult = device.acquireNextImageKHR(
            info.swapchain, std::numeric_limits<std::uint64_t>::max(), info.swapchainSemaphore, {});
        swapchainImageIndex = acquireResult.value;
        info.stale = acquireResult.result == vk::Result::eSuboptimalKHR;
    }
    catch (const vk::OutOfDateKHRError&)
    {
        info.stale = true;
        return;
    }

    device.resetFences(info.renderFence);
    device.resetCommandPool(info.commandPool);

    const auto swapchainImage = info.images[swapchainImageIndex];
    auto cmd = info.commandBuffer;
    cmd.begin({vk::CommandBufferUsageFlagBits::eOneTimeSubmit});

    const vk::ImageSubresourceRange subresourceRange{
        vk::ImageAspectFlagBits::eColor, 0, vk::RemainingMipLevels, 0, vk::RemainingArrayLayers
    };
    const auto transition = [&](const vk::ImageLayout from, const vk::ImageLayout to)
    {
        vk::ImageMemoryBarrier2 barrier{};
        barrier.setSrcStageMask(vk::PipelineStageFlagBits2::eAllCommands)
               .setSrcAccessMask(vk::AccessFlagBits2::eMemoryWrite)
               .setDstStageMask(vk::PipelineStageFlagBits2::eAllCommands)
               .setDstAccessMask(vk::AccessFlags2{vk::AccessFlagBits2::eMemoryWrite} | vk::AccessFlagBits2::eMemoryRead)
               .setOldLayout(from)
               .setNewLayout(to)
               .setImage(swapchainImage)
               .setSubresourceRange(subresourceRange);
        vk::DependencyInfo dependencyInfo{};
        dependencyInfo.setImageMemoryBarriers(barrier);
        cmd.pipelineBarrier2(dependencyInfo);
    };

    transition(vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal);

    vk::RenderingAttachmentInfo colorAttachment{};
    colorAttachment.setImageView(info.imageViews[swapchainImageIndex])
                   .setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
                   .setLoadOp(vk::AttachmentLoadOp::eClear)
                   .setStoreOp(vk::AttachmentStoreOp::eStore)
                   .setClearValue(vk::ClearValue{vk::ClearColorValue{std::array<float, 4>{0.07f, 0.07f, 0.09f, 1.0f}}});
    vk::RenderingInfo renderingInfo{};
    renderingInfo.setRenderArea(vk::Rect2D{{0, 0}, info.extent}).setLayerCount(1).setColorAttachments(colorAttachment);
    cmd.beginRendering(renderingInfo);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), static_cast<VkCommandBuffer>(cmd));
    cmd.endRendering();

    transition(vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR);
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

    try
    {
        const auto presentResult = queue.presentKHR(presentInfo);
        if (presentResult == vk::Result::eSuboptimalKHR)
        {
            info.stale = true;
        }
        else
        {
            checkResult(presentResult, "Failed to present");
        }
    }
    catch (const vk::OutOfDateKHRError&)
    {
        info.stale = true;
    }
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
    vk::PhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.setSynchronization2(true).setDynamicRendering(true);
    deviceCreateInfo.setPEnabledFeatures(&deviceFeatures);
    const std::array deviceExtensions{static_cast<const char*>(vk::KHRSwapchainExtensionName)};
    deviceCreateInfo.setPEnabledExtensionNames(deviceExtensions);
    deviceCreateInfo.setPNext(&vulkan13Features);

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
                                       WindowFlags::Visible | WindowFlags::Resizable);
    initVulkanWindow(windowId);
    imguirwin::init(windowId);
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
    std::vector<WindowEvent> events{};
    events.resize(64);
    auto quit = false;
    const auto autoMaximizeSeconds = std::getenv("RWIN_TEST_AUTOMAX") ? std::atof(std::getenv("RWIN_TEST_AUTOMAX")) : 0.0;
    const auto loopStart = std::chrono::steady_clock::now();
    auto autoMaximized = false;
    while (!quit)
    {
        if (autoMaximizeSeconds > 0.0 && !autoMaximized &&
            std::chrono::duration<double>(std::chrono::steady_clock::now() - loopStart).count() > autoMaximizeSeconds)
        {
            maximizeWindow(windowId);
            autoMaximized = true;
        }
        pumpEvents();
        const auto eventsGotten = getEvents(events);
        for (auto i = 0; i < eventsGotten; i++)
        {
            const auto& event = events[i];
            if (event.info.windowId != windowId) continue;

            if (event.info.type == WindowEventType::Close)
            {
                quit = true;
            }
            imguirwin::handleEvent(event);
        }
        drawWindow(windowId);
    }
    device.waitIdle();
    ImGui_ImplVulkan_Shutdown();
    ImGui::DestroyContext();
    device.destroyDescriptorPool(imguiPool);
    destroyVulkanWindow(windowId);
    destroyWindow(windowId);
    destroyVulkan();
    return 0;
}
