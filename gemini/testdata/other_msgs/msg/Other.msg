Header header       # offset 0 (length 16)

int32 CONST = 1234
string SCONST = string constant

string bar      # offset 16
int32 value     # offset 20
int64[] arr     # offset 24 (length 8)
int8[10] farr   # offset 32 (length 10)
 
Enum en         # offset 42
# final offset 43 (aligned to 44)


