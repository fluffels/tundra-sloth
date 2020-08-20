#pragma warning(disable: 4267)

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

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
    initVKPipeline(
        vk,
        "default",
        VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        pipeline
    );

    int heightMapWidth, heightMapDepth, n;
    unsigned char *data = stbi_load(
        "textures/noise.png",
        &heightMapWidth,
        &heightMapDepth,
        &n,
        1
    );

    VulkanMesh mesh;
    mesh.vCount = heightMapWidth * heightMapDepth;
    vector<Vertex> vertices;
    const float offset = -heightMapWidth / 2.f;
    int i = 0;
    for (int z = 0; z < heightMapDepth; z++) {
        for (int x = 0; x < heightMapWidth; x++) {
            auto& v = vertices.emplace_back();
            v.position.x = offset + x;
            v.position.z = offset + z;
            v.position.y = 15.f * -data[i] / 255.f;
            i++;
        }
    }
    stbi_image_free(data);

    vector<uint32_t> indices;
    for (int z = 0; z < heightMapDepth - 1; z++) {
        for (int x = 0; x < heightMapWidth - 1; x++) {
            i = z * heightMapDepth + x;
            indices.push_back(i);
            indices.push_back(i+1);
            indices.push_back(i+heightMapDepth);
            indices.push_back(i+heightMapDepth);
            indices.push_back(i+1);
            indices.push_back(i+1+heightMapDepth);
        }
    }

    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        vertices.data(),
        vertices.size()*sizeof(Vertex),
        indices.data(),
        indices.size()*sizeof(uint32_t),
        mesh
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
            &mesh.vBuff.handle,
            offsets
        );
        vkCmdBindIndexBuffer(
            cmd,
            mesh.iBuff.handle,
            0,
            VK_INDEX_TYPE_UINT32
        );
        vkCmdDrawIndexed(
            cmd,
            indices.size(),
            1, 0, 0, 0
        );

        vkCmdEndRenderPass(cmd);

        checkSuccess(vkEndCommandBuffer(cmd));
    }
}
