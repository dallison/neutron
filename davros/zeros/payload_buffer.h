#pragma once

#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <string_view>

namespace davros::zeros {

constexpr uint32_t kFixedBufferMagic = 0x65766144;
constexpr uint32_t kMovableBufferMagic = 0x45766144;

using BufferOffset = uint32_t;

struct FreeBlockHeader {
  uint32_t length;
  BufferOffset next;
};

struct VectorHeader {
  uint32_t num_elements;
  BufferOffset data;
};

// This is a buffer that holds the contents of a message.
// It is located at the first address of the actual buffer with the
// reset of the buffer memory following it.
//
struct PayloadBuffer {
  uint32_t magic;         // Magic to identify wireformat.
  uint32_t hwm;           // Offset one beyond the highest used.
  uint32_t full_size;     // Full size of buffer.
  BufferOffset metadata;  // Offset to message metadata.
  BufferOffset free_list; // Heap free list.
  BufferOffset message;   // Offset for the message.

  // Initialize a new PayloadBuffer at this with a message of the
  // given size.
  PayloadBuffer(uint32_t size, bool is_fixed = true)
      : magic(is_fixed ? kFixedBufferMagic : kMovableBufferMagic), hwm(0),
        full_size(size), metadata(0) {
    InitFreeList();
  }

  // Allocate space for the main message in the buffer and set the
  // 'message' field to its offset.
  static void *AllocateMainMessage(PayloadBuffer **self, size_t size);

  // Allocate space for the message metadata and copy it in.
  static void AllocateMetadata(PayloadBuffer **self, void *md, size_t size);

  // Allocate space for the string. 
  // 'old_offset' is the offset into the buffer of the 'pointer'
  // to the string data.
  // The string is copied in.
  static char *SetString(PayloadBuffer **self, const std::string &s,
                              BufferOffset old_offset = 0);

  template <typename T> void Set(BufferOffset offset, T v);
  template <typename T> T Get(BufferOffset offset);

  template <typename T>
  static void VectorPush(PayloadBuffer **self, VectorHeader *hdr, T v);

  // 'offset' is the offset into the buffer of the 'pointer'
  // to the string data.
  std::string GetString(BufferOffset offset) const {
    return GetString(ToAddress<const BufferOffset>(offset));
  }
  std::string_view GetStringView(BufferOffset offset) const {
    return GetStringView(ToAddress<const BufferOffset>(offset));
  }

  std::string GetString(const BufferOffset *addr) const;
  std::string_view GetStringView(const BufferOffset *addr) const;

  template <typename T>
  T VectorGet(const VectorHeader *hdr, size_t index) const;

  void Dump(std::ostream &os);
  void DumpFreeList(std::ostream &os);

  void InitFreeList();
  FreeBlockHeader *FreeList() { return ToAddress<FreeBlockHeader>(free_list); }

  // Allocate some memory in the buffer.  The buffer might move.
  static void *Allocate(PayloadBuffer **buffer, uint32_t n, uint32_t alignment,
                        bool clear = true);
  void Free(void *p);
  static void *Realloc(PayloadBuffer **buffer, void *p, uint32_t n,
                       uint32_t alignment, bool clear = true);

  template <typename T = void> T *ToAddress(BufferOffset offset) {
    if (offset == 0) {
      return nullptr;
    }
    return reinterpret_cast<T *>(reinterpret_cast<char *>(this) + offset);
  }

  template <typename T = void> BufferOffset ToOffset(T *addr) {
    if (addr == reinterpret_cast<T *>(this) || addr == nullptr) {
      return 0;
    }
    return reinterpret_cast<char *>(addr) - reinterpret_cast<char *>(this);
  }

  template <typename T = void> const T *ToAddress(BufferOffset offset) const {
    if (offset == 0) {
      return nullptr;
    }
    return reinterpret_cast<const T *>(reinterpret_cast<const char *>(this) +
                                       offset);
  }

  template <typename T = void> BufferOffset ToOffset(const T *addr) const {
    if (addr == reinterpret_cast<const T *>(this) || addr == nullptr) {
      return 0;
    }
    return reinterpret_cast<const char *>(addr) -
           reinterpret_cast<const char *>(this);
  }

  void InsertNewFreeBlockAtEnd(FreeBlockHeader *free_block,
                               FreeBlockHeader *prev, uint32_t length);

  void MergeWithAboveIfPossible(FreeBlockHeader *alloc_block,
                                FreeBlockHeader *alloc_header,
                                FreeBlockHeader *free_block,
                                BufferOffset *next_ptr, size_t alloc_length);

  void ShrinkBlock(FreeBlockHeader *alloc_block, uint32_t orig_length,
                   uint32_t new_length, uint32_t *len_ptr);

  uint32_t *MergeWithFreeBlockBelow(void *alloc_block, FreeBlockHeader *prev,
                                    FreeBlockHeader *free_block,
                                    uint32_t new_length, uint32_t orig_length,
                                    bool clear);

  void ExpandIntoFreeBlockAbove(FreeBlockHeader *free_block,
                                uint32_t new_length, uint32_t len_diff,
                                uint32_t free_remaining, uint32_t *len_ptr,
                                BufferOffset *next_ptr, bool clear);
  uint32_t TakeStartOfFreeBlock(FreeBlockHeader *block, uint32_t num_bytes,
                                uint32_t full_length, FreeBlockHeader *prev);

  void UpdateHWM(void *p) { UpdateHWM(ToOffset(p)); }

  void UpdateHWM(BufferOffset off) {
    if (off > hwm) {
      hwm = off;
    }
  }
};

template <typename T> inline void PayloadBuffer::Set(BufferOffset offset, T v) {
  T *addr = ToAddress<T>(offset);
  *addr = v;
}

template <typename T> inline T PayloadBuffer::Get(BufferOffset offset) {
  T *addr = ToAddress<T>(offset);
  return *addr;
}

template <typename T>
inline void PayloadBuffer::VectorPush(PayloadBuffer **self, VectorHeader *hdr,
                                      T v) {
  // hdr points to a VectorHeader:
  // uint32_t num_elements;     - number of elements in the vector
  // BufferOffset data;         - BufferOffset to vector contents
  // The vector contents is allocated in the buffer.  It is preceded
  // by the block size (in bytes).
  uint32_t total_size = hdr->num_elements * sizeof(T);
  if (hdr->data == 0) {
    // The vector is empty, allocate it with a default size of 2 and 8 byte
    // alignment.
    void *vecp = Allocate(self, 2 * sizeof(T), 8);
    hdr->data = (*self)->ToOffset(vecp);
  } else {
    // Vector has some values in it.  Retrieve the total size from
    // the alllcated block header (before the start of the memory)
    uint32_t *block = (*self)->ToAddress<uint32_t>(hdr->data);
    uint32_t current_size = block[-1];
    if (current_size == total_size) {
      // Need to double the size of the memory.
      void *vecp = Realloc(self, block, 2 * hdr->num_elements * sizeof(T), 8);
      hdr->data = (*self)->ToOffset(vecp);
    }
  }
  // Get address of next location in vector.
  T *valuep = (*self)->ToAddress<T>(hdr->data) + hdr->num_elements;
  // Assign value.
  *valuep = v;
  // Increment the number of elements.
  hdr->num_elements++;
}

template <typename T>
inline T PayloadBuffer::VectorGet(const VectorHeader *hdr, size_t index) const {
  if (index >= hdr->num_elements) {
    return static_cast<T>(0);
  }
  const T *addr = ToAddress<const T>(hdr->data);
  if (addr == nullptr) {
    return static_cast<T>(0);
  }
  return addr[index];
}

} // namespace davros::zeros
