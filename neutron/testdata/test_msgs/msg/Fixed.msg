// Only fixed size field types are allowed here.
// Strings and vectors are disallowed.  String constants are OK.
int32 foo
uint8[10] name
SubFixed sub
SubFixed[4] asub
int8 i8
Enum8 e8
Enum16 e16
Enum32 e32
Enum64 e64
float32[10] afloat
float64[20] adouble
Enum8[3] ae8
Enum16[3] ae16
Enum32[3] ae32
Enum64[3] ae64
bool b
bool[2] ab
duration d
time t
duration[2] ad
time[2] at

# Constants
int8 const1 = 56
float32 const2 = 99
string const3 = foo bar
