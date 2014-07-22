redo-ifchange compiler_flags
. ./compiler_flags
file=${1#build/}
gcc $CFLAGS -o $3 -c ../src/${file%.o}.c -MD -MF $2.deps
read DEPS <$2.deps
redo-ifchange ${DEPS#*:}
