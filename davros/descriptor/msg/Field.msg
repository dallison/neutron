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



