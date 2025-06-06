#pragma once
#include "Cafe/HW/Latte/ISA/LatteReg.h"
#include "util/math/vector2.h"

class VKRMoveableRefCounterRef
{
public:
	class VKRMoveableRefCounter* ref;
};

class VKRMoveableRefCounter
{
public:
	VKRMoveableRefCounter()
	{
		selfRef = new VKRMoveableRefCounterRef();
		selfRef->ref = this;
	}

	virtual ~VKRMoveableRefCounter()
	{
		cemu_assert_debug(m_refCount == 0);

		// remove references
#ifdef CEMU_DEBUG_ASSERT
		for (auto itr : refs)
		{
			auto& rev = itr->ref->reverseRefs;
			rev.erase(std::remove(rev.begin(), rev.end(), this->selfRef), rev.end());
		}
#endif
		for (auto itr : refs)
		{
			itr->ref->m_refCount--;
			if (itr->ref->m_refCount == 0)
				itr->ref->RefCountReachedZero();
		}
		refs.clear();
		delete selfRef;
		selfRef = nullptr;
	}

	VKRMoveableRefCounter(const VKRMoveableRefCounter&) = delete;
	VKRMoveableRefCounter& operator=(const VKRMoveableRefCounter&) = delete;
	VKRMoveableRefCounter(VKRMoveableRefCounter&& rhs) noexcept
	{
		this->refs = std::move(rhs.refs);
		this->m_refCount = rhs.m_refCount;
		rhs.m_refCount = 0;
		this->selfRef = rhs.selfRef;
		rhs.selfRef = nullptr;
		this->selfRef->ref = this;
	}

	VKRMoveableRefCounter& operator=(VKRMoveableRefCounter&& rhs) noexcept
	{
		cemu_assert(false);
		return *this;
	}

	void addRef(VKRMoveableRefCounter* refTarget)
	{
		this->refs.emplace_back(refTarget->selfRef);
		refTarget->m_refCount++;

#ifdef CEMU_DEBUG_ASSERT
		// add reverse ref
		refTarget->reverseRefs.emplace_back(this->selfRef);
#endif
	}

	// methods to directly increment/decrement ref counter (for situations where no external object is available)
	void incRef()
	{
		m_refCount++;
	}

	void decRef()
	{
		m_refCount--;
		if (m_refCount == 0)
			RefCountReachedZero();
	}

protected:
  	virtual void RefCountReachedZero()
	{
		// does nothing by default
	}

	int m_refCount{};
private:
	VKRMoveableRefCounterRef* selfRef;
	std::vector<VKRMoveableRefCounterRef*> refs;
#ifdef CEMU_DEBUG_ASSERT
	std::vector<VKRMoveableRefCounterRef*> reverseRefs;
#endif

	void moveObj(VKRMoveableRefCounter&& rhs)
	{
		this->refs = std::move(rhs.refs);
		this->m_refCount = rhs.m_refCount;
		this->selfRef = rhs.selfRef;
		this->selfRef->ref = this;
	}
};

class VKRDestructibleObject : public VKRMoveableRefCounter
{
public:
	void flagForCurrentCommandBuffer();
	bool canDestroy();

	virtual ~VKRDestructibleObject() {};
private:
	uint64 m_lastCmdBufferId{};
};

/* Vulkan objects */

class VKRObjectTexture : public VKRDestructibleObject
{
public:
	VKRObjectTexture();
	~VKRObjectTexture() override;

	VkImage m_image{ VK_NULL_HANDLE };
	VkFormat m_format = VK_FORMAT_UNDEFINED;
	VkImageCreateFlags m_flags;
	VkImageAspectFlags m_imageAspect;
	struct VkImageMemAllocation* m_allocation{};

};

class VKRObjectTextureView : public VKRDestructibleObject
{
public:
	VKRObjectTextureView(VKRObjectTexture* tex, VkImageView view);
	~VKRObjectTextureView() override;

	VkImageView m_textureImageView{ VK_NULL_HANDLE };
	VkSampler m_textureDefaultSampler[2] = { VK_NULL_HANDLE, VK_NULL_HANDLE }; // relict from LatteTextureViewVk, get rid of it eventually
};


class VKRObjectSampler : public VKRDestructibleObject
{
  public:
	VKRObjectSampler(VkSamplerCreateInfo* samplerInfo);
	~VKRObjectSampler() override;

	static VKRObjectSampler* GetOrCreateSampler(VkSamplerCreateInfo* samplerInfo);
	static void DestroyCache();

	void RefCountReachedZero() override; // sampler objects are destroyed when not referenced anymore

	VkSampler GetSampler() const { return m_sampler; }
  private:
	static std::unordered_map<uint64, VKRObjectSampler*> s_samplerCache;
	VkSampler m_sampler{ VK_NULL_HANDLE };
	uint64 m_hash;
};

class VKRObjectRenderPass : public VKRDestructibleObject
{
public:
	struct AttachmentEntryColor_t
	{
		VKRObjectTextureView* viewObj{};
		bool isPresent{};
		VkFormat format; // todo - we dont need to track isPresent or viewObj, we can just compare this with VK_FORMAT_UNDEFINED
	};

	struct AttachmentEntryDepth_t
	{
		VKRObjectTextureView* viewObj{};
		bool isPresent{};
		VkFormat format; // todo - we dont need to track isPresent or viewObj, we can just compare this with VK_FORMAT_UNDEFINED
		bool hasStencil;
	};

	struct AttachmentInfo_t
	{
		AttachmentEntryColor_t colorAttachment[Latte::GPU_LIMITS::NUM_COLOR_ATTACHMENTS];
		AttachmentEntryDepth_t depthAttachment;
	};

	VkFormat GetColorFormat(size_t index)
	{
		if (index >= Latte::GPU_LIMITS::NUM_COLOR_ATTACHMENTS)
			return VK_FORMAT_UNDEFINED;
		return m_colorAttachmentFormat[index];
	}

	VkFormat GetDepthFormat()
	{
		return m_depthAttachmentFormat;
	}

public:
	VKRObjectRenderPass(AttachmentInfo_t& attachmentInfo, sint32 colorAttachmentCount = Latte::GPU_LIMITS::NUM_COLOR_ATTACHMENTS);
	~VKRObjectRenderPass() override;
	VkRenderPass m_renderPass{ VK_NULL_HANDLE };
	VkFormat m_colorAttachmentFormat[Latte::GPU_LIMITS::NUM_COLOR_ATTACHMENTS];
	VkFormat m_depthAttachmentFormat;
	uint64 m_hashForPipeline; // helper var. Holds hash of all the renderpass creation parameters (mainly the formats) that affect the pipeline state
};

class VKRObjectFramebuffer : public VKRDestructibleObject
{
public:
	VKRObjectFramebuffer(VKRObjectRenderPass* renderPass, std::span<VKRObjectTextureView*> attachments, Vector2i size);
	~VKRObjectFramebuffer() override;

	VkFramebuffer m_frameBuffer{ VK_NULL_HANDLE };
};

class VKRObjectPipeline : public VKRDestructibleObject
{
public:
	VKRObjectPipeline();
	~VKRObjectPipeline() override;

	void SetPipeline(VkPipeline newPipeline);
	VkPipeline GetPipeline() const { return m_pipeline; }

	VkDescriptorSetLayout m_vertexDSL = VK_NULL_HANDLE, m_pixelDSL = VK_NULL_HANDLE, m_geometryDSL = VK_NULL_HANDLE;
	VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;

private:
	VkPipeline m_pipeline = VK_NULL_HANDLE;
};

class VKRObjectDescriptorSet : public VKRDestructibleObject
{
public:
	VKRObjectDescriptorSet();
	~VKRObjectDescriptorSet() override;

	VkDescriptorSet descriptorSet{ VK_NULL_HANDLE };
};