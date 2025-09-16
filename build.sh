# ====================================================================
#  $File: build.sh $
#  $Date: 16-09-2025 @ 10-39-45 $
#  $Revision: 0.0.0 $
#  $Creator: aeternatrix $
#  $Notice: (C) Copyright 2025 by aeternatrix. All Rights Reserved. $
# ====================================================================

#!/bin/bash

src="src/*"
out_fd="bin"
out_exe="libfsti.so"
out="-o ${out_fd}/${out_exe}"

mkdir -p $out_fd

echo -e "\033[33m===== GCC =====\033[0m"

gcc -fPIC -shared $out $src

echo -e "\033[33m===============\033[0m"
