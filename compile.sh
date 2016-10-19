#/bin/bash

RED='\033[1;31m'
GREEN='\033[1;32m'
NC='\033[0m'

Comp=""
if [ -z "$COMPILER" ]; then
    Comp="/usr/bin/c++"
else
    Comp=$COMPILER
fi

LLVM="$LLVM_DIR/install"
BISON="${BISON_DIR}/install"

VER="CMAKE_NO_VERBOSE=1"

ResultFile="last"
CompiledFile="k.out"

while [[ $# > 0 ]]; do
    key=$1
    case $key in
        -v|--verbose)
            VER="VERBOSE=1"
        ;;
        -c|--clean)
            CLE="true"
        ;;
        -j|--jobs)
            JOBS=$2
        ;;
        -t|--test)
            UNIT_TEST="true"
        ;;
    esac
    shift
done

if [ "$CLE" == "true" ]; then
    rm -rf build/
fi

mkdir -p build && pushd build && cmake -DCMAKE_PREFIX_PATH="${LLVM};${BISON}" -DCMAKE_CXX_COMPILER="$Comp" .. && make $VER "-j$JOBS" && popd
if [ ! -f build/koon ]; then
    exit
fi

if [ "$UNIT_TEST" != "true" ]; then
    exit
fi
mkdir -p "test" && cp "build/koon" "test/koon" && pushd "test"
endstring="Result: "
diffFile="difres"
for testDoc in *.k
do
    ./koon -o $CompiledFile $testDoc
    ./$CompiledFile `cat $testDoc.args` > $ResultFile

    ret=$?
    diff -s $ResultFile $testDoc.out > $diffFile
    out=$?

    if [ $ret -eq `cat $testDoc.ret` ] && [ $out -eq 0 ]; then
        endstring="$endstring${GREEN}P"
    else
        echo -e "\n============================\n"
        echo "test: " $testDoc
        endstring="$endstring${RED}F"
        echo -e "Koon's ret =" $ret "\nNormal ret =" `cat $testDoc.ret`
        cat $diffFile
        echo -e "\n============================\n"
    fi
    rm -f $CompiledFile $ResultFile
done

endstring="$endstring${NC}"
echo -e $endstring
