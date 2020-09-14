#pragma warning(disable: 4267)

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "MathLib.h"
#include "RenderLevel.h"

struct Vertex {
    Vec3 position;
};

static int heightMapWidth;
static int heightMapDepth;
static VulkanMesh mesh;
static vector<Vertex> vertices;

void renderLevel(
    Vulkan& vk,
    vector<VkCommandBuffer>& cmds
) {
    VulkanPipeline meshPipeline;
    initVKPipeline(
        vk,
        "default",
        meshPipeline
    );
    VulkanPipeline wireframePipeline;
    initVKPipeline(
        vk,
        "default",
        wireframePipeline
    );

    int n;
    unsigned char *data = stbi_load(
        "textures/noise.png",
        &heightMapWidth,
        &heightMapDepth,
        &n,
        1
    );

    mesh.vCount = heightMapWidth * heightMapDepth;
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

    vector<uint32_t> meshIndices;
    vector<uint32_t> wireframeIndices;
    for (int z = 0; z < heightMapDepth - 1; z++) {
        for (int x = 0; x < heightMapWidth - 1; x++) {
            i = z * heightMapDepth + x;
            meshIndices.push_back(i);
            meshIndices.push_back(i+1);
            meshIndices.push_back(i+heightMapDepth);
            meshIndices.push_back(i+heightMapDepth);
            meshIndices.push_back(i+1);
            meshIndices.push_back(i+1+heightMapDepth);

            wireframeIndices.push_back(i);
            wireframeIndices.push_back(i+1);
            wireframeIndices.push_back(i+heightMapDepth);
            wireframeIndices.push_back(i+1);
            wireframeIndices.push_back(i+1+heightMapDepth);
            wireframeIndices.push_back(0xFFFFFFFF);
        }
    }

    uploadMesh(
        vk.device,
        vk.memories,
        vk.queueFamily,
        vertices.data(),
        vertices.size()*sizeof(Vertex),
        meshIndices.data(),
        meshIndices.size()*sizeof(uint32_t),
        mesh
    );

    VulkanBuffer wireframeIndexBuffer;
    uploadIndexBuffer(
        vk.device,
        vk.memories,
        vk.queueFamily,
        wireframeIndices.data(),
        wireframeIndices.size()*sizeof(uint32_t),
        wireframeIndexBuffer
    );

    updateUniformBuffer(
        vk.device,
        meshPipeline.descriptorSet,
        0,
        vk.mvp.handle
    );
    updateUniformBuffer(
        vk.device,
        wireframePipeline.descriptorSet,
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
            meshPipeline.handle
        );
        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            meshPipeline.layout,
            0,
            1,
            &meshPipeline.descriptorSet,
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
        Vec3 white = {1, 1, 1};
        vkCmdPushConstants(
            cmd,
            meshPipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(white),
            &white
        );
        vkCmdDrawIndexed(
            cmd,
            meshIndices.size(),
            1, 0, 0, 0
        );

        vkCmdBindPipeline(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            wireframePipeline.handle
        );
        Vec3 blue = {0, 0, 1};
        vkCmdPushConstants(
            cmd,
            wireframePipeline.layout,
            VK_SHADER_STAGE_VERTEX_BIT,
            0,
            sizeof(blue),
            &blue
        );
        vkCmdBindIndexBuffer(
            cmd,
            wireframeIndexBuffer.handle,
            0,
            VK_INDEX_TYPE_UINT32
        );
        vkCmdDrawIndexed(
            cmd,
            wireframeIndices.size(),
            1, 0, 0, 0
        );

        vkCmdEndRenderPass(cmd);

        checkSuccess(vkEndCommandBuffer(cmd));
    }
}
