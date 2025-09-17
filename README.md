# file_structure_template_implementer
A C shared object that builds a project structure. Almost certainly does not work on Windows.

## Building
Run the build.sh script or

```sh
mkdir -p bin
gcc -fPIC -shared src/* -o bin/libfsti.so
```

## Using the shared object
The function to call is:
```c
void run_structure(char* structure_file, char* template_directory, char* root);
```

Some examples on how to call it:

lua:
```lua
local ffi = require("ffi")

ffi.cdef[[
    void run_structure(const char* structure_file, const char* template_directory, const char* root);
]]
local fsti = ffi.load("path/to/libfsti.so")
fsti.run_structure("path/to/your/file.structure", "/path/to/your/templates/", "/path/to/the/destination/root/")
```
python:
```py
from ctypes import *

fsti = CDLL("path/to/libfsti.so")
fsti.run_structure(b"path/to/your/file.structure", b"/path/to/your/templates/", b"/path/to/the/destination/root/")
```

## Structure File
Format:
```
.
+-- file <options>
+-- file.ext <options>
+-- file.ext
+-- .file
+-- folder
    +-- subfile <options>
    +-- subfile.ext <options>
    +-- subfile.ext
    +-- .subfile
    +-- subfolder
--- <name> <occurrence>
//-- <template name> <opt>
<content>
```
Note: \<opt> is only used with exactly 'comment_header' see below.

### Options
\<options> is to do with file read/write/execute permissions. The default permissions are:
- user  (u) read/write (r/w)
- group (g) read/write (r/w)
- other (o) read       (r/w)

The valid characters are: ugoa+-rwx

It is read left to right; when ugoa is not specified, it applies to all (a); when +- is not specified, it assumes + until otherwise specified.

### Example
c.structure (I give them the ext .structure, this is not enforced in any way)
```
.
+-- build.sh x
+-- header
+-- src
    +-- main.c
--- build.sh
//-- comment_header #
//-- build.sh
--- main.c
//-- comment_header //
#include <stdio.h>

int main() {
    printf("Hello, World!\n");
    return 0;
}
```

#### Comment Header
If the template called is exactly "comment_header" it will scan the template contents for the following, and replace them:
- %FILE% → the name of the file
- %DATE% → the current date and time (%d-%m-%Y @ %H-%M-%S)
- %YEAR% → the current year (%Y)
- %ME%   → the user (acquired from $HOME environment variable)

Additionally, it will prepend all lines with the appropriate comment string provided (\<opt>). If no opt is provided, it will insert a space.
