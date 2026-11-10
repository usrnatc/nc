#!/bin/bash

YELLOW='\033[1;33m'
RED='\033[0;31m'
GREEN='\033[0;32m'
RESET='\033[0;0m'

BuildMode="debug"
GuiDefine="-DNC_GUI=1 -DNC_USE_OPENGL -DNC_USE_FREETYPE -DNC_SLOW_FLOATS"

for arg in "$@"; do
    case "$(echo $arg | tr '[:upper:]' '[:lower:]')" in
        release)
            BuildMode="release"
            ;;
        dev)
            BuildMode="dev"
            ;;
    esac
done

mkdir -p build
pushd build > /dev/null

MATHASMSourceFiles="../internal/linux/linux_math.asm"
INTRINASMSourceFiles="../internal/linux/linux_intrin.asm"
ASMFlags="-f elf64"
MATHASMOutputObj=linux_math.o
INTRINASMOutputObj=linux_intrin.o

SourceFiles="../main.cpp"
OutputBin="main"

SharedCompilerFlags=(
    "-std=c++14"
    "-w"
    "-fno-rtti"
    "-fno-exceptions"
    "-fpermissive"
    "-mavx2"
    "-mssse3"
    "-msse4.1"
    "-msse4.2"
    "-mavx"
    "-msse"
    "-msse2"
    "-mbmi"
    "-fno-stack-protector"
    "-fno-strict-aliasing"
    "-fno-stack-protector"
    "-fcf-protection=none"
    "-fno-sanitize=all"
    "-fno-threadsafe-statics"
    "-fno-use-cxa-atexit"
    "-fno-implicit-templates"
    "-fno-asynchronous-unwind-tables"
    "-fno-unwind-tables"
    "-fno-exceptions"
    "-I../external"
    "-I../internal"
    "-I../cache"
    "-I/usr/include/freetype2"
    "-I/usr/include/libpng16"
    "-DUNICODE"
    "-D_GNU_SOURCE"
)

WarningFlags=(
    "-Wno-sign-compare"
    "-Wno-unused-parameter"
    "-Wno-unused-function"
    "-Wno-unused-parameter"
    "-Wno-unused-variable"
    "-Wno-unused-function"
    "-Wno-sign-compare"
    "-Wno-missing-field-initializers"
    "-Wno-missing-braces"
    "-Wno-unused-but-set-variable"
    "-Wno-type-limits"
    "-Wno-switch"
    "-Wno-return-type"
    "-Wno-parentheses"
    "-Wno-unknown-pragmas"
    "-Wno-attributes"
    "-Wno-write-strings"
)

SharedLinkerFlags=(
    "-lpthread"
    "-ldl"
    "-lm"
    "-static-libgcc"
    "-static-libstdc++"
    "-Wl,-Bstatic -lfreetype"
    "-Wl,-Bstatic -lpng"
    "-Wl,-Bstatic -lz"
    "-Wl,-Bstatic -lbz2"
    "-Wl,-Bstatic -lpng16"
    "-Wl,-Bstatic -lbrotlidec"
    "-Wl,-Bstatic -lbrotlicommon"
    "-Wl,-Bstatic -lstdc++"
    "-Wl,-Bdynamic -ldl"
)

# GCC needs these at the end of the call bc its linker only does a single pass
# TODO: change these to static (except GL) when on a distro that does not suck balls
GUILinkerFlags=(
    "-Wl,-Bdynamic -lX11"
    "-Wl,-Bdynamic -lXext"
    "-Wl,-Bdynamic -lxcb"
    "-Wl,-Bdynamic -lXau"
    "-Wl,-Bdynamic -lXdmcp"
    "-Wl,-Bdynamic -lXrandr"
    "-Wl,-Bdynamic -lXfixes"
    "-Wl,-Bdynamic -lGL"
    "-Wl,-Bdynamic -lEGL"
)

case "$BuildMode" in
    debug)
        echo -e "${YELLOW}____building_debug___________${RESET}"
        ConfigCompilerFlags="-O0 -g -D_DEBUG -DNC_DEBUG=1"
        PostBuildCmd="./$OutputBin"
        ;;
    release)
        echo -e "${YELLOW}____building_release_________${RESET}"
        ConfigCompilerFlags="-O3 -DNDEBUG"
        PostBuildCmd="./$OutputBin"
        ;;
    dev)
        echo -e "${YELLOW}____building_dev_debug_______${RESET}"
        ConfigCompilerFlags="-O0 -g -pg -D_DEBUG -DNC_DEBUG=1"
        PostBuildCmd="gdb ./$OutputBin"
        ;;
    *)
        echo -e "${RED}Invalid build mode: $BuildMode${RESET}"
        popd > /dev/null
        exit 1
        ;;
esac

echo -e "${YELLOW}____deleting_build_data______${RESET}"
rm -f "$OutputBin"
rm -f *.o

echo -e "${YELLOW}____assembling_linux_asms____${RESET}"
nasm "$ASMFlags" "$MATHASMSourceFiles" -o "$MATHASMOutputObj"
nasm "$ASMFlags" "$INTRINASMSourceFiles" -o "$INTRINASMOutputObj"

echo -e "${YELLOW}____compiling_and_linking____${RESET}"
g++ ${SharedCompilerFlags[@]} ${WarningFlags[@]} $ConfigCompilerFlags \
    $GuiDefine $MrfDefine $SourceFiles $MATHASMOutputObj $INTRINASMOutputObj \
    -o $OutputBin \
    ${SharedLinkerFlags[@]} ${GUILinkerFlags[@]}

LastError=$?

if [ $LastError -ne 0 ]; then
    echo -e "\n${RED}BUILD FAILED.${RESET}"
    popd > /dev/null
    exit $LastError
fi

echo -e "${GREEN}____finished_________________${RESET}"

if [ -n "$PostBuildCmd" ] && [ "$BuildMode" != "dev" ]; then
    echo -e "${YELLOW}____running_post_build_______${RESET}"
    $PostBuildCmd
elif [ "$BuildMode" == "dev" ]; then
    echo -e "${YELLOW}____dev mode: run 'gdb ./$OutputBin' to debug____${RESET}"
fi

popd > /dev/null
