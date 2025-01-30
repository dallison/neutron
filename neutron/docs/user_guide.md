# Neutron User Guide

Neutron is a ROS message parser and generator.  It takes regular ROS (version 1)
messages and generates C++ structs corresponding to that message.  There's no
point in having structs if you can't send them anywhere, so Neutron also generates a couple of ways to send the generated struct over a wire.

1. Serialized ROS message format (serdes)
2. Zero-copy message format (zeros)

## ROS messages
ROS messaging is pretty primitive by any standards.  The input IDL is a flat set of fields looking something like this:

```
int32 field1
string field2
int64[] vector
string[32] array
Message msg
```

Each field has a type and a name.  The square brackets indicate that the field is an fixed or variable sized array of such types. The type can be another message defined in the same IDL. There are also constants available.  Aside from allowing multiple messages in a single file for RPC, that's about it.

The output wire format is simplistic too.  Each field is written into a buffer sequentially in its little-endian format.  A 64-bit int is written as a little-endian (on all modern systems) 64-bit integer; a string is a 32-bit length followed by a sequence of bytes (no zero at the end); a fixed size array is just a sequence of the types; a varible sized array has a 32-bit length (number of elements) before the sequence of items.  Finally, a message is just the message contents themselves and constants are not present.

Compared to a modern wireformat, like Google's Protocol Buffers (protobuf), ROS's encoding results in data about 4 times the length it needs to be and doesn't allow for optional fields or any type of backward or forward compatibility.

The C++ struct corresponding to the message is a straight translation into C++ types.  Integer fields are translated directly; strings are `std::string`, arrays are either `std::array` or `std::vector`; messages are the inline message type; and constants are generated as an enumerated type.

Traditionally, ROS code would manipulate these structs directly with no accessor functions.

# Input files
ROS message files are always placed in a directory called `msg` and always have the file extension `.msg`.  The parent directory of `msg` is known as a `package`.  There may be multiple packages defined in a project.

If a field refers to a message in another package, the system needs to be able to find that package.  That is specified as a command line argument to the generator program: list of paths to find imported messages.

# Serialization and deserialization (serdes)
A ROS node sees the C++ (or whatever other language) structs.  It either fills in the fields if it is sending a message, or accesses them if it is reading one.

If the node wants to send the data over an IPC system it needs to flattened into a format that can be sent serially over a wire (serialized).  Serialization
takes each field in turn, in the order specified in the IDL input and writes it to a buffer in memory.  This copies all the memory in the struct to a buffer.  It is then in a form that can be sent, byte-by-byte, over to another process in a different address space.

When the message is received by a node it must first be converted from its serialized format into something that the node can use.  This is the reverse of serialization: deserialization and results in another copy of the received data to heap-allocated memory.

Neutron's serialization system is 100% wire compatible with all other ROS serialization systems and thus messages can be exchanged with any traditional ROS (version 1, not DDS), system.

## Generated files
The input .msg files are convered to two C++ files.  Say the input .msg file is Foo.msg:

1. A header file named `Foo.h`
2. A C++ source file named `Foo.cc`

The path for the generated files includes the name of the package.

For example, if the input file is `my_msgs/msg/Foo.msg`, the output files are (on a mac):

1. bazel-out/darwin_arm64-fastbuild/bin/neutron/serdes/my_msgs/Foo.h
2. bazel-out/darwin_arm64-fastbuild/bin/neutron/serdes/my_msgs/Foo.cc

They are always placed in `neutron/serdes`.

The generated messages are placed in a namespace, which is the same as the package name.  There is also the ability to append another namespace after the package name if both serialized and zero-copy messages present in the same program.  By default, the name `serdes` is appended to the pacakge to form the namespace, but this can be changed at build time (or omitted).

For example, given a message in the file `my_msgs/Foo.msg`:

```
int8 OK = 1
uint64 seq
ik_msgs/Pose[] poses
```

the following will be generated in `neutron/serdes/my_msgs/Foo.h`:

```c++
#pragma once

#include "neutron/serdes/ik_msgs/Pose.h"

namespace my_msgs::serdes {
struct Foo {
  static constexpr int8_t OK = 1;
  uint64_t seq = {};
  std::vector<ik_msgs::serdes::Pose> poses = {};
};
}
```


## Generated C++ struct
Messages are defined as a standalone `struct`.  Each field is defined as a member of the struct with the same name as the field.  If the field name is a C++ keyword, the field name has an underscore appended to it.

Native alignment is used (no packing specified).

If a message contains only constants, it is defined as an `enum class` with the base type derived from the size of the contants and the enumerations containing all the constant values.

### Integers and floating point
No surprises, direct mapping between the types in the .msg file and their C++ counterparts.  For example:

```
int32 a
uint16 b
float64 c
```

results in the members:

```c++
int32_t a;
uint32_t b;
double c;
```


### Strings
These are `std::string` instances.  For example:

```
string name
```

results in:

```c++
std::string name;
```

### Arrays (fixed size)
`std::array<T, Size>` where `T` is the array type and `Size` is the array length from the `.msg` file.  For example:

```
string[32] names
```

results in:

```c++
std::array<std::string, 32> names;
```

### Vectors
`std::vector<T>`.  Like arrays.  For example:

```
float32[] values
```

results in:

```c++
std::vector<double> values;
```

### Messages
Instance of the `struct` defined for the message (the header file is automatically included).  For example, given a message called `ik_msgs/Pose.msg`, we can use it in another message:

```
ik_msgs/Pose start_pose
```

resulting in:

```c++
#include "neutron/serdes/ik_msgs/Pose.h"
...

ik_msgs::serdes::Pose start_pose;
```

### Time and Duration
These are defined as two included structs `neutron::Time` and `neutron::Duration` that are compatible with the ROS equivelents.

```c++
struct Time {
  uint32_t secs;
  uint32_t nsecs;
  bool operator==(const Time &t) const {
    return secs == t.secs && nsecs == t.nsecs;
  }
  bool operator!=(const Time &t) const { return !this->operator==(t); }
};

struct Duration {
  uint32_t secs;
  uint32_t nsecs;

  bool operator==(const Duration &d) const {
    return secs == d.secs && nsecs == d.nsecs;
  }
  bool operator!=(const Duration &d) const { return !this->operator==(d); }
};
```

### Constants
A constant has a type and a value.  They are output as a `static constexpr` in the struct definition.  Non-string constants are defined as follows:

```
int8 FULL = 1
```

```c++
static constexpr int8_t FULL = 1;
```

And strings are defined as:

```
string UNKNOWN = unknown value provided
```

```c++
static inline const char UNKNOWN[] = "unknown value provided";
```

## Member functions
In addition to the struct definition, the following member functions are generated for a message (where MESSAGE is the message type name)

```c++
  static const char* Name() { return "MESSAGE"; }
  static const char* FullName() { return "PACKAGE/MESSAGE"; }
  absl::Status SerializeToArray(char* addr, size_t len) const;
  absl::Status SerializeToBuffer(neutron::serdes::Buffer& buffer) const;
  absl::Status DeserializeFromArray(const char* addr, size_t len);
  absl::Status DeserializeFromBuffer(neutron::serdes::Buffer& buffer);
  size_t SerializedSize() const;
  bool operator==(const MESSAGE& m) const;
  bool operator!=(const MESSAGE& m) const {
    return !this->operator==(m);
  }
  std::string DebugString() const;
```

The `Name()` and `FullName()` static functions give you the local and full names respectively.  You serialize the message to an array or a buffer by calling `SerializeToArray` and `SerializedToBuffer`.  A `Buffer` is defined below.

To deserialize from wire format, use `DeserializeFromArray` or `DeserializeFromBuffer`.  The size of the serialized wireformat data can be obtained by calling `SerializedSize`.  This can be used before the message is serialized in order to determine how much memory will be consumed by the wireformat data.

The `DebugString` function gives you a human readable string for debugging.

## Descriptor
The  `descriptor` for a message contains the metadata for the message.  This is defined as a message with the definition:

`msg/Descriptor.msg`
```
string package
string name
string[] imports
Field[] fields
```

`msg/Field.msg`
```
# Field types
uint8 TYPE_INT8 = 1
uint8 TYPE_UINT8 = 2
uint8 TYPE_INT16 = 3
uint8 TYPE_UINT16 = 4
uint8 TYPE_INT32 = 5
uint8 TYPE_UINT32 = 6
uint8 TYPE_INT64 = 7
uint8 TYPE_UINT64 = 8
uint8 TYPE_FLOAT32 = 9
uint8 TYPE_FLOAT64 = 10
uint8 TYPE_STRING = 11
uint8 TYPE_TIME = 12
uint8 TYPE_DURATION = 13
uint8 TYPE_BOOL = 13
uint8 TYPE_MESSAGE = 14

int16 FIELD_PRIMITIVE = -2
int16 FIELD_VECTOR = -1

int16 index
string name
uint8 type
int16 array_size    # -2: not an array, -1: vector: n: fixed

# For message fields.
string msg_package    # Message field package
string msg_name       # Message field message name
```

Thus an instance of the descriptor describes all the fields and types in a message.  Each message contains such a descriptor in its `_descriptor` member, which is serialized `Descriptor.msg` message.  In order to obtain a deserialized version of a message's descriptor, deserialize the `_descriptor` static member in the desired message type.

## Wire Format
When the message is serialized, it is written into a buffer.  There is no alignment of any field type.  Message fields are written in the order specified in the `.msg` file.

### Integers
These are written in little-endian format

### Floating point
Written in binary in little-endian format.

### Strings.
Written as little-endian 32-bit length followed immediately by the bytes of the string, with no terminating zero byte.

### Arrays (fixed size)
Written as a contiguous sequence of the array type.

### Vectors (variable size)
Written as a little-endian 32-bit number of elements followed by a contiguous sequence of elements.

### Messages
Written as a full inline message (all the message fields) in sequence.

### Time and Duration
These are written as two 32-bit little endian fields.

### Constants
Not present in the wire-format.

# Zero copy
While serializing messages to and from heap-memory is usually OK, there are certain types of messages where it becomes onerous and starts consuming a lot of CPU cycles.  If your messages contain large flat fields, like camera images or LIDAR point clouds these can occupy a lot of memory (megabytes per image) and the memory copy becomes expensive.

What you really want is the ability to create the serialized message directly from the camera or LIDAR data rather than copying it to or from a heap-allocated vector.  If your camera driver supports it, it would be really cool if you could tell the driver to puts its data at an address that is inside the buffer you want to send.  If this buffer is in shared memory, you can completely avoid any copies.  Even if your camera provides a buffer that you don't control, at least you should be able to avoid one of the copies.

So, how do you do this?  The fact that ROS nodes access the struct fields directly and not via accessor functions makes this much harder to do.  The node wants to see an `std::string`, or `std::vector` or whatever other C++ data structure.  If it called accessor functions (like protobuf does), a zero-copy
messaging system could do whatever it likes in these functions (see [Phaser](https://github.com/dallison/phaser) for software that does exactly that for protobuf).

The solution lies in C++ operator overloading and conversions.  If the struct that the node is accessing contains operator overloads for assigning and converting to and from the C++ types, we can trick the node into thinking that it is accessing fields directly, when it is actually calling accessor functions.

This trick allows us to place the actual data for the field in a different location and relay to it without the node being aware of it.

Let's take an example.  Say we have a message named `Image` that has a 32-bit integer field called `size`.  A regular ROS message would define the struct as:

```c++
struct Message {
  int32_t size;
};
```

However, if we define the C++ struct that the node sees contains the following:

```c++
struct SizeField {
  operator int32_t() const;
  int32_t operator=(int32_t v);
};

struct Image {
  SizeField size;
};
```

The node can access the `size` field as it would normally but instead of accessing the field directly from memory, it would call one of the two operators
defined in `SizeField`.  So the following code would work without any modifications to the node:

```c++
int32_t s = msg->size;
msg->size = 1234;
```

The first statement calling the `operator int32_t() const` function and the second calling `int32_t operator=(int32_t)`.

This allows the message generator to provide whatever functionality it needs inside the functions without the node ever knowing about it.

## Creating a zero copy message
Before trying to create a zero-copy message for sending you have to consider where the data will be held.  In a real robot, the only good reason to use zero-copy is if you have a shared memory IPC system that allows you to access the IPC buffer directly (a great example of this is [Subspace](https://github.com/dallison/subspace)).

To create a `mutable` message in a buffer, use the static `CreateMutable` function generated for the message class.  You then can fill in the message and then determine its size using the `Size` function.  The address of the buffer in which the message resides can be obtained by calling `Buffer` on the message.

For example, say you have an IPC buffer with size 1MB and a message called `my_msgs/Image` you want to create in it:

```c++
auto msg = my_msgs::zeros::Image::CreateMutable(ipc_buffer, 1024*1024);
// Fill in the message
SendMesssage(msg.Buffer(), msg.Size());

```

When a message is received in a buffer, you access it directly by creating an instance of the message using the static function `CreateReadonly`, giving it the buffer and size received.

```c++
auto msg = my_msgs::zeros::Image::CreateReadonly(buffer, message_size);
// Access the fields of the message directly.
```

There are also functions to create zero-copy messages in a heap-allocated block of memory if you need to do so.  They are:

1. `CreateDynamicMutable(size_t initial_size, std::function<absl::StatusOr<void*>(size_t)> alloc, std::function<void(void*)> free,std::function<absl::StatusOr<void*>(void*, size_t, size_t)> realloc)`
2. `CreateDynamicMutable(size_t initial_size = 1024)`

The `initial_size` parameter is the size of the initial malloced block of memory.  The first variant allows you to pass functions to allocated, free and reallocate the memory if straight `malloc`, `free` and `realloc` are not to your liking.

## Serializing to/from ROS wire format
You can transcode a `zeros` message into and out of serialized ROS message format using the `SerializeToArray` and `DeserializeFromArray` functions.  You can also get the serialized size (before you serialize) by calling `SerializedSize`


## Generated files
The input .msg files are convered to two C++ files.  Say the input .msg file is Foo.msg:

1. A header file named `Foo.h`
2. A C++ source file named `Foo.cc`

The path for the generated files includes the name of the package.

For example, if the input file is `my_msgs/msg/Foo.msg`, the output files are (on a mac):

1. bazel-out/darwin_arm64-fastbuild/bin/neutron/zeros/my_msgs/Foo.h
2. bazel-out/darwin_arm64-fastbuild/bin/neutron/zeros/my_msgs/Foo.cc

They are always placed in `neutron/zeros`.

The generated messages are placed in a namespace, which is the same as the package name.  There is also the ability to append another namespace after the package name if both serialized and zero-copy messages present in the same program.  By default, the name `zeros` is appended to the pacakge to form the namespace, but this can be changed at build time (or omitted).

For example, given a message in the file `my_msgs/Foo.msg`:

```
int8 OK = 1
uint64 seq
ik_msgs/Pose[] poses
```

the following will be generated in `neutron/zeros/my_msgs/Foo.h`:

```c++
#pragma once

#include "neutron/zeros/ik_msgs/Pose.h"

namespace my_msgs::zeros {
struct Foo {
  static constexpr int8_t OK = 1;
  neutron::zeros::Uint64Field seq = {};
  neutron::zeros::MessageVectorField<ik_msgs::serdes::Pose> poses = {};
};
}
```

## Generated class
Every message generated by Neurton's `zeros` is a subclass of a class called [Message](../zeros/message.h).  The generated class does not contain the actual values of the fields, but instead, for each field, it contains an instance of a class that redirects reads and writes to the actual location of the field in a buffer.

The `Message` base class contains a `std::shared_ptr` to the buffer containing the actual message contents and the offset of the message into that buffer.

If a message contains only constants, it is defined as an `enum class` with the base type derived from the size of the contants and the enumerations containing all the constant values.

The classes for the fields contain two 32-bit integer members:

1. `source_offset`: a positive offset from the start of the struct to the start of the field's member in the struct
2. `binary_offset`: a positive offset from the start of the wire-format (binary) message to the start of the field within the wire-format message

The `source_offset` member allows the field class to locate the start of the struct in order to find the address of the binary message in a buffer

The `binary_offset` is added to the binary message's start address to form the address of the actual data for the field in the binary message.

The idea is that the struct generated by the Neutron zeros compiler can be accessed by a program forming or reading a message as if it is accessing a normal struct with members.  In actual operation, accesses to the members of the struct are redirected to a buffer containing the actual binary values of the fields.

This diagram shows the general layout of a message. 

<div>
<img src="Neutron message layout.png">
</div>

### Integer and floating point fields
Each integer type has a corresponding class defined for it whose name is the Pascal-case message type name followed by `Field`.  For example:

```
int8 -> Int8Field
uint32 -> Uin32Field
float64 -> Float64Field
```

### Strings
These are defined as `StringField`.

### Enums
A field that refers to a message that contains only constants is defined as an `EnumNField`, where `N` is 8, 16, 32 or 64.  For example, `Enum16Field`.

### Messages
These are defined as an instance of a templated class `MessageField` with the template argument being the type of the message's struct.

### Arrays (fixed size)
The class type depends on what the base type of the array is.  Each is a template with an argument `N` specifying the size of the array.

1. Integers, floating points: `PrimitiveArrayField<T, N>` where `T` is the C++ type corresponding to the message field.
2. Enums: `EnumArrayField<E, N>` where `E` is the enumeration type
3. Strings: `StringArrayField<N>`
4. Messages: `MessageArrayField<M, N>` where `M` is the message type.
5. Time and Duration: `PrimitiveArrayField<neutron::Time, N>` or `PrimitiveArrayField<neutron::Duration, N>`

### Vectors
These are almost identical to arrays, with the omission of the `N` for the size, since the size is defined as runtime.

1. Integers, floating points: `PrimitiveVectorField<T>` where `T` is the C++ type corresponding to the message field.
2. Enums: `EnumVectorField<E>` where `E` is the enumeration type
3. Strings: `StringVectorField`
4. Messages: `MessageVectorField<M>` where `M` is the message type.
5. Time and Duration: `PrimitiveVectorField<neutron::Time>` or `PrimitiveVectorField<neutron::Duration>`
  
## Binary field formats
Whereas the source message is what the user's program interacts with, the location
of the actual message field values is stored in a [PayloadBuffer](https://github.com/dallison/cpp_toolbelt/toolbelt/payload_buffer.h).

This is C++ class that provides a heap inside a relocatable buffer.  The buffer's memory
can be either provided by the user or allocated by Neutron.  If it is allocated
by Neutron, it is variable in size.  If it is allocated by the user it is generally
fixed size (in shared memory for example), but a resizer can be provided to change
the size.

The `PayloadBuffer` works by creating a simple malloc/free data structure inside the
memory provided.  It is not a standard heap management system though because 
of the requirement that it can't use any virtual addresses (pointers).

There are two types of allocators in the `PayloadBuffer`, the choice of which
to use being based on the block being allocated.  If the block is small
a `bitmap allocator` is used to speed up the allocation.  The definition of
a small block is one whose size is less than 16, 32, 64 or 128 bytes.  There
are 4 bitmap allocators, one for each of those block sizes.  Bitmap
allocators are fast but use more memory because they allocate a set of
fixed size blocks up front and keep track of them using a bit mask.  This uses
more memory than a simple free-list allocator.

The second type of allocator is used for all other block sizes above
128 bytes and is a simple free-list based allocator.  The bitmap allocator
prevents heap fragmentation by using fixed size blocks, but with the free-list
allocator, fragmentation is always possible.

You can switch the bitmap allocator off when creating the `PayloadBuffer` using
its constructor. 


### PayloadBuffer header
The binary for a message is located in a `PayloadBuffer`.  The start of every PayloadBuffer contains a header, shown in the following diagram:


<div>
<img src="PayloadBuffer header.png"/>
</div>

Each member is 4-bytes long.  The first member is the `magic` which is either:

1. toolbelt::kFixedBufferMagic
2. toolbelt::kMovableBufferMagic

This serves to identify the buffer as a PayloadBuffer and also whether it is
movable or fixed.  Movable buffers are only moved when creating the message, however
all buffers are relocatable in memory.

The bottom bit of the magic field is used to specify whether the `bitmap allocator`
is on or off (0 = off).

The next member, `message` contains the offset to the root of the message tree
in the buffer - the top level message.

Then we have the `hwm` or high water mark which specifies the current size of the
data allocated in the buffer, and thus the size of the memory that needs to be
sent to a receiver.

This is followed by the current full size of the buffer, which can change if the
buffer is resized.  If you try to allocate too much memory in a fixed size buffer
the program will abort to prevent memory overwrites.

Then we have the offset to the first free block in the buffer.  Each free block consists
of a header followed by some free bytes.  The header is a struct:

```c++
struct FreeBlockHeader {
  uint32_t length;   // Length of the free block, including the header.
  BufferOffset next; // Absolute offset into buffer for next free block.
};
```

The memory in the buffer is organized as an ordered linked list of free blocks, interspersed
by allocated blocks.  An allocated block is always prefixed by a 4-byte length, which does not
include the length field itself.  The allocator does its best to keep the free list as short as 
possible to coalescing adjacent free blocks and expanding into blocks if possible.

Then we have a user-supplied offset to arbitrary `metadata`.  This is not used by
Neutron, but can be provided by the user to make additional data available as necessary.
For example, it could be set to the name of the message, or some other descriptor
that can be used by a receiver.

Finally we have 4 offsets for the 4 bitmap run vectors.  Each offset corresponds to a bitmap run for a
particular size (16, 32, 64 or 128 bytes).  The bitmap runs are held in a vector of offsets to
`BitMapRun` structs that grows as needed.  Each `BitMapRun` struct holds a bitmask showing
which blocks are allocated, meta information about the run and the blocks themselves, each
of which is prefixed by a 4 byte header.  If a block is allocated in a run, its corresponding bit in the
bitmask is set to 1.

This is shown in the following diagram, for a 32 byte run.

<div>
<img src="Bitmap run.png"/>
</div>

The binary version of a message can be seen in the following diagram:

<div>
<img src="Neutron binary message.png"/>
</div>


The start of the message consists of the fixed size portion of the message.  This
has space for every field in the message.  Primitive fields (like int32, float64, etc.)
occupy space that is sized and aligned appropriately for the type.  Variable sized
fields (strings, messages, arrays and vectors) consist of a header that
refers to another location in the PayloadBuffer that contains the actual contents of the field.

Please see the file [runtime.h](../zeros/runtime.h) for detailed information about how fields are stored. 

### Strings and bytes
These fields have a 4-byte header containing the offset (relative to the
start of the PayloadBuffer) of the string (or bytes) data.  The data consists
of a 4-byte length followed immediately by the actual string contents.  Strings
are not zero-terminated.  If the header contains the value 0, the string field
is not present.

### Message fields
These have a header that contains the offset (relative to the PayloadBuffer) of another
message, which is in the format described here.  Like strings, a value of 0 means
that the field is not present.

### Arrays
These are held as a contiguous sequence of inline elements.  If the type is `array of message` the array contains the fixed size portion of the message and the variable sized portion is allocated elsewhere in the buffer.

### Vectors
The header for these is 8-bytes long and contains:

1. The number of elements in the vector
2. The offset (relative to the PayloadBuffer) of the contiguous memory for the fields

The data for the vector is held contiguously in memory and can be moved if the
vector is expanded.  The contents of the memory depends on the type of the
repeated field.  The `capacity` of the vector (the amount of memory allocated) is
held immediately before the data and says how many bytes are allocated, not elements as
in the header (this is because it's just the size of the block allocated by
the allocator).

### Buffer expansion
The intention of zero-copy systems like Neutron is to allow you to create messages
directly in destination memory (like a Subspace IPC buffer).  This will allow very
high performance messaging.  However, if you just want to use the regular protobuf
algorithms, Neutron provides an expandable PayloadBuffer that grows as necessary 
to accommodate the message.  This is useful if you are creating messages that will
be sent over a network and there is still an advantage of zero-copy since you
eliminate serialization.

If you create a message that uses the default constructor, Neutron will allocate
a PayloadBuffer from the heap and provide an internal resize function that will
reallocate the buffer if you try to allocate more memory than is available.  This
will likely move the PayloadBuffer in the heap (it uses realloc).  Neutron will
take care of any buffer moves internally, but if you are calling `Allocate` to
allocate any memory yourself (for metadata perhaps), you need to be aware that
any pointers you retain from before the allocation might not be valid after
the reallocation if the buffer moves.  It's best to convert them to offsets
as those are always valid.
