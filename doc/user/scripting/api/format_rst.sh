#!/usr/bin/env sh

rst_file="$1"
out_file="$2"

#-e 's,Scheme Procedure: \*\*\(.*\)\*\* | \*\(\(\w*\).*\)*\*,\.\. code-block:: scheme\n\n   (\1 \2)\n,g' \
  #-e 's,   \([^(]\),\1,g' \

sed \
  -e 's,Scheme Procedure: \*\*\(.*\)\*\* | \*\(\(\w*\).*\)*\*,``(\1 \2)``,g' \
  -e 's,Scheme Procedure: \*\*\(.*\)\*\*,``(\1)``,g' \
  "$rst_file" > "$out_file"
