#pragma warning(disable: 4267)

#include "MathLib.h"
#include "Vulkan.h"

struct Vertex {
    Vec3 position;
};

void renderLevel(
    Vulkan& vk,
    vector<VkCommandBuffer>& cmds
) {
    VulkanPipeline pipeline;
    initVKPipeline(vk, "default", pipeline);

    Vertex vertices[] = {
        { 0, -1, 0 },
        { 2, 1, 0 },
        { -1, 1, 0 }
    };

    VulkanMesh defaultMesh;
    defaultMesh.vCount = 3;
    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        vertices,
        3*sizeof(Vertex),
        defaultMesh
    );

    updateUniformBuffer(
        vk.device,
        pipeline.descriptorSet,
        0,
        vk.mvp.handle
    );

    uint32_t framebufferCount = vk.swap.images.size();
    cmds.resize(framebufferCount);
    createCommandBuffers(vk.device, vk.cmdPool, framebufferCount, cmds);
    for (size_t swapIdx = 0; swapIdx < framebufferCount; swapIdx++) {
        auto& cmd = cmds[swapIdx];
        beginFrameCommandBuffer(cmd);

        VkClearValue colorClear;
        colorClear.color = {};
        VkClearValue depthClear;
        depthClear.depthStencil = { 1.f, 0 };
        VkClearValue clears[] = { colorClear, depthClear };

        VkRenderPassBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginInfo.clearValueCount = 2;
        beginInfo.pClearValues = clears;
        beginInfo.framebuffer = vk.swap.framebuffers[swapIdx];
        beginInfo.renderArea.extent = vk.swap.extent;
        beginInfo.renderArea.offset = {0, 0};
        beginInfo.renderPass = vk.renderPass;

        vkCmdBeginRenderPass(cmd, &beginInfo, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.handle
        );
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.layout,
            0,
            1,
            &pipeline.descriptorSet,
            0,
            nullptr
        );
        VkDeviceSize offsets[] = {0};

        vkCmdBindVertexBuffers(
            cmd,
            0, 1,
            &defaultMesh.vBuff.handle,
            offsets
        );
        vkCmdDraw(
            cmd,
            defaultMesh.vCount, 1,
            0, 0
        );

        vkCmdEndRenderPass(cmd);

        checkSuccess(vkEndCommandBuffer(cmd));
    }
}
