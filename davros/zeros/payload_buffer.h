#pragma once

#include <iostream>
#include <stdint.h>
#include <stdlib.h>
#include <string>

namespace davros::zeros {

constexpr uint32_t kFixedBufferMagic = 0x65766144;
constexpr uint32_t kMovableBufferMagic = 0x45766144;

using BufferOffset = uint32_t;

struct FreeBlockHeader {
  uint32_t length;
  BufferOffset next;
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

  // Allocate space for the string.  If 'old' is non-zero it contains the
  // address of 4 bytes holding the current offset to the string
  // The string is copied in.
  static char *AllocateString(PayloadBuffer **self, const std::string &s,
                                    uint32_t* old = nullptr);

  template <typename T>
  static void VectorPush(PayloadBuffer **self, void *vec, T v) {
    // vec points to:
    // uint32_t n;     - number of elements in the vector
    // uint32_t v;     - BufferOffset to vector contents
    // The vector contents is allocated in the buffer.  It is preceded
    // by the block size (in bytes).
    uint32_t *p = reinterpret_cast<uint32_t *>(vec);
    uint32_t num = p[0];
    BufferOffset value = p[1];
    uint32_t total_size = num * sizeof(T);
    if (value == 0) {
      // The vector is empty, allocate it with a default size of 2.
      void *vecp = Allocate(self, 2 * sizeof(T), 8);
      p[1] = (*self)->ToOffset(vecp);
    } else {
      // Vector has some values in it.  Retrieve the total size from
      // the alllcated block header (before the start of the memory)
      uint32_t *block = (*self)->ToAddress<uint32_t>(value);
      uint32_t current_size = block[-1];
      if (current_size == total_size) {
        // Need to double the size of the memory.
        void *vecp = Realloc(self, block, 2 * num, 8);
        p[1] = (*self)->ToOffset(vecp);
      }
    }
    // Get address of next location in vector.
    T *valuep = (*self)->ToAddress<T>(p[1]);
    // Assign value.
    *valuep = v;
    // Increment the number of elements.
    p[1]++;
  }

  void Dump(std::ostream &os);
  void DumpFreeList(std::ostream &os);

  void InitFreeList();
  FreeBlockHeader *FreeList() { return ToAddress<FreeBlockHeader>(free_list); }

  // Allocate some memory in the buffer.  The buffer might move.
  static void *Allocate(PayloadBuffer **buffer, uint32_t n, uint32_t alignment, bool clear = true);
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
    if (addr == reinterpret_cast<T *>(this)) {
      return 0;
    }
    return reinterpret_cast<char *>(addr) - reinterpret_cast<char *>(this);
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
                                    uint32_t new_length, uint32_t orig_length);

  void ExpandIntoFreeBlockAbove(FreeBlockHeader *free_block,
                                uint32_t new_length, uint32_t len_diff,
                                uint32_t free_remaining, uint32_t *len_ptr,
                                BufferOffset *next_ptr);
  uint32_t TakeStartOfFreeBlock(FreeBlockHeader *block, uint32_t num_bytes,
                                uint32_t full_length, FreeBlockHeader *prev);

  void UpdateHWM(void *p) { UpdateHWM(ToOffset(p)); }

  void UpdateHWM(BufferOffset off) {
    if (off > hwm) {
      hwm = off;
    }
  }
};

} // namespace davros::zeros
