CUDA_ROOT=/usr/local/cuda

# Linux
#LIB=-L$(CUDA_ROOT)/lib64 -lcudart

# OSX
MAC_CFLAGS=#-framework OpenGL -framework GLUT 
LIB=#-L$(CUDA_ROOT)/lib -lcudart -framework OpenGL -framework GLUT 
#$(MAC_CFLAGS)

EXE=vdxf
all: $(EXE) $(GTEST)

#
# Shouldn't need to change anything below this line
#
SRCS=util.c sdict.c dxf_types.c dxf.c vdxf.c
COMMON_OBJ=util.o sdict.o dxf_types.o dxf.o 
OBJ=vdxf.o $(COMMON_OBJ)
INC=-I/usr/local/cuda/include
DEBUG=-g #-DNDEBUG
CFLAGS=-c90 -Wall -Wextra -Wno-long-long -pedantic $(INC) $(DEBUG) $(MAC_CFLAGS)
CUDAFLAGS=--compiler-options "$(CFLAGS)" -m64 --ptxas-options=-v
LDFLAGS=-g $(LIB)

check:
	cppcheck *.c
	splint -onlytrans -usereleased -compdestroy -temptrans -branchstate +posixlib *.c

.SUFFIXES: .cu .cuh 

.cu.o:
	nvcc $(CUDAFLAGS) -c $<

$(EXE):$(OBJ)
	$(CC) $(LDFLAGS) -o $@ $(OBJ)

clean:
	rm -f *.o $(EXE)

doc:
	doxygen

depend:
	makedepend -- $(CFLAGS) -- -Y $(SRCS)

# DO NOT DELETE

sdict.o: sdict.h
dxf_types.o: dxf_types.h
dxf.o: dxf.h util.h dxf_types.h
vdxf.o: dxf.h util.h sdict.h
