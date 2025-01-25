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

The output wire format is simplistic too.  Each field is written into a buffer sequentially in its native format.  A 64-bit int is written as a little-endian (on all modern systems) 64-bit integer; a string is a 32-bit length followed by a sequence of bytes (no zero at the end); a fixed size array is just a sequence of the types; a varible sized array has a 32-bit length (number of elements) before the sequence of items.  Finally, a message is just the message contents themselves and constants are not present.

Compared to a modern wireformat, like Google's Protocol Buffers (protobuf), ROS's encoding results in messagers about 4 times the length they need to be and doesn't allow for optional fields or any type of backward or forward compatibility.

The C++ struct corresponding to the message is a straight translation into C++ types.  Integer fields are translated directly; strings are `std::string`, arrays are either `std::array` or `std::vector`; messages are the inline message type; and constants are generated as an enumerated type.

Traditionally, ROS code would manipulate these structs directly with no accessor functions.

# Serialization and deserialization
A ROS node sees the C++ (or whatever other language) structs.  It either fills in the fields if it is sending a message, or accesses them if it is reading one.

If the node wants to send the data over an IPC system it needs to flattened into a format that can be sent serially over a wire (serialized).  Serialization
takes each field in turn, in the order specified in the IDL input and writes it to a buffer in memory.  It is then in a form that can be sent, byte-by-byte, over to another process in a different address space.

When the message is received by 