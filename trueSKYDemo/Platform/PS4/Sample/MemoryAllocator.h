#include "Simul/Base/RuntimeError.h"
#include "Simul/Base/MemoryInterface.h"
class AllocatorMemoryInterface:public simul::base::MemoryInterface
{
	sce::Gnmx::Toolkit::IAllocator *gpu_allocator;
	sce::Gnmx::Toolkit::IAllocator *shared_allocator;
	std::map<void*,size_t> bytes_allocated;
	std::map<void*,std::string> callers;
	std::map<void*,size_t> video_bytes_allocated;
	std::map<void*,std::string> video_callers;
	std::map<std::string,int> memoryTracks;
public:
	AllocatorMemoryInterface(sce::Gnmx::Toolkit::IAllocator *g,sce::Gnmx::Toolkit::IAllocator *sh)
		:gpu_allocator(g),shared_allocator(sh)
	{
	}
	~AllocatorMemoryInterface()
	{
		if(bytes_allocated.size())
		{
			std::cerr<<"Unfreed general allocations: "<<bytes_allocated.size()<<std::endl;
			for(std::map<void*,std::string>::iterator i=callers.begin();i!=callers.end();i++)
			{
				std::cerr<<"0x"<<std::hex<<i->first<<": "<<i->second.c_str()<<" "<<bytes_allocated[i->first]<<" bytes"<<std::endl;
			}
		}
		if(video_bytes_allocated.size())
		{
			std::cerr<<"Unfreed video allocations: "<<video_bytes_allocated.size()<<std::endl;
			for(std::map<void*,std::string>::iterator i=video_callers.begin();i!=video_callers.end();i++)
			{
				std::cerr<<"0x"<<std::hex<<i->first<<": "<<i->second.c_str()<<" "<<video_bytes_allocated[i->first]<<" bytes"<<std::endl;
			}
		}
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void* AllocateTracked(size_t nbytes,size_t align,const char *fn) override
	{
#if 1
		return memalign(align,nbytes);
#else
		void *ptr;
		if(align==0)
			align=1;
		ptr=shared_allocator->allocate(nbytes,(sce::Gnm::AlignmentType)align);
		if(fn)
		{
			//memalign(align,nbytes);//
			if(ptr==(void*)0x21362f580)
			{
				if(bytes_allocated.find(ptr)!=bytes_allocated.end())
					SIMUL_ASSERT(false);
			}
			bytes_allocated[ptr]=nbytes;
			callers[ptr]		=std::string(fn);
		}
		return ptr;
#endif
	}
	//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
	virtual void Deallocate(void* ptr) override
	{
#if 1
		free(ptr);
#else
		if(ptr)
		{
			if(callers.find(ptr)==callers.end())
			{
				return;
			}
			bytes_allocated.erase(bytes_allocated.find(ptr));
			callers.erase(callers.find(ptr));
			shared_allocator->release(ptr);
		}
#endif
	}
	//! Allocate \a nbytes bytes of memory, aligned to \a align and return a pointer to them.
	virtual void* AllocateVideoMemoryTracked(size_t nbytes,size_t align,const char *fn) override
	{
		void *ptr;
		if(align==0)
			align					=1;
		ptr							=gpu_allocator->allocate(nbytes,(sce::Gnm::AlignmentType)align);
		if(fn)
		{
			if(video_callers.find(ptr)!=video_callers.end())
			{
				std::cerr<<"Already got this address from "<<video_callers[ptr].c_str()<<std::endl;
			}
			if(ptr==(void*)0x215e20000)
			{
				video_bytes_allocated[ptr]=nbytes;
			}
			video_bytes_allocated[ptr]	=nbytes;
			video_callers[ptr]			=std::string(fn);
			auto i=memoryTracks.find(fn);
			if(i==memoryTracks.end())
			{
				memoryTracks[fn]=0;
				i=memoryTracks.find(fn);
			}
			i->second+=nbytes;
			video_callers[ptr]=fn;
		}
		return ptr;
	}
	//! De-allocate the memory at \param address (requires that this memory was allocated with Allocate()).
	virtual void DeallocateVideoMemory(void* address) override
	{
		if(address)
		{
			if(address==(void*)0x215e20000)
			{
				video_bytes_allocated.find(address);
			}
			if(video_callers.find(address)==video_callers.end())
			{
				//std::cerr<<"Not got this address"<<std::endl;
				return;
			}
			video_bytes_allocated.erase(video_bytes_allocated.find(address));
			gpu_allocator->release(address);
			int size=bytes_allocated[address];
			auto n=video_callers.find(address);
			if(n!=video_callers.end())
			{
				const std::string &name=n->second;
				if(name.length())
				{
					auto i=memoryTracks.find(name);
					if(i!=memoryTracks.end())
					{
						i->second-=size;
						if(i->second<=0)
							video_callers.erase(n);
					}
				}
			}
			bytes_allocated.erase(address);
		}
	}

	const char *GetNameAtIndex(int index) const override
	{
		std::map<std::string,int>::const_iterator i=memoryTracks.begin();
		for(int j=0;j<index&&i!=memoryTracks.end();j++)
		{
			i++;
		}
		if(i==memoryTracks.end())
			return nullptr;
		return i->first.c_str();
	}
	int GetBytesAllocated(const char *name) const override
	{
		if(!name)
			return 0;
		auto i=memoryTracks.find(name);
		if(i==memoryTracks.end())
			return 0;
		return i->second;
	}
	int GetTotalBytesAllocated() const override
	{
		return 0;
	}
	virtual int GetTotalVideoBytesAllocated() const override
	{
		int bytes=0;
		for(auto i:memoryTracks)
		{
			bytes+=i.second;
		}
		return bytes;
	}
	virtual int GetCurrentVideoBytesAllocated() const
	{
		return 0;
	}
	virtual int GetTotalVideoBytesFreed() const
	{
		return 0;
	}
};