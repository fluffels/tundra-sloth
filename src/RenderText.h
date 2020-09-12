#include <vector>

#include "Vulkan.h"

using namespace std;

void recordTextCommandBuffers(Vulkan& vk, vector<VkCommandBuffer>& cmds, char* text);
void resetTextCommandBuffers(Vulkan& vk, vector<VkCommandBuffer>& cmds);
