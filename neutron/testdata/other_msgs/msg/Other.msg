Header header       # offset 0 (length 16)

int32 CONST = 1234
string SCONST = string constant

string bar      # offset 16
int32 value     # offset 20
int64[] arr     # offset 24 (length 4)
int8[10] farr   # offset 28 (length 10)
 
Enum en         # offset 38
