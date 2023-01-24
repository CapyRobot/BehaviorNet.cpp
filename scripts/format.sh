#/bin/bash

find . -not -path "./src/3rd_party/*" -iname *.hpp -o -iname *.cpp | xargs clang-format -i
