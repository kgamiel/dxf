CUDA_ROOT=/usr/local/cuda
GTK_ROOT=/opt/local/include/gtk-2.0

# Linux
LIB=-L. -L/opt/local/lib #-L$(CUDA_ROOT)/lib64 -lcudart
CFLAGS=-I$(GTK_ROOT)

# OSX
#CFLAGS=-c90 
#LIB=#-L$(CUDA_ROOT)/lib -lcudart -framework OpenGL -framework GLUT 

EXE=vdxf
LIBRARY=libdxf.a

all: $(EXE) $(GTEST)

#
# Shouldn't need to change anything below this line
#
SRCS=util.c sdict.c dxf_types.c dxf.c vdxf.c
LIB_OBJ=util.o sdict.o dxf_types.o dxf.o 
EXE_OBJ=vdxf.o
INC=-I/usr/local/cuda/include
DEBUG=-g #-DNDEBUG
CFLAGS+=-Wall -Wextra -Wno-long-long -pedantic $(INC) $(DEBUG)
CUDAFLAGS=--compiler-options "$(CFLAGS)" -m64 --ptxas-options=-v
LDFLAGS=-g $(LIB)

check:
	cppcheck *.c
	splint -onlytrans -usereleased -compdestroy -temptrans -branchstate +posixlib *.c

.SUFFIXES: .cu .cuh 

.cu.o:
	nvcc $(CUDAFLAGS) -c $<

$(LIBRARY):$(LIB_OBJ)
	ar -crv $(LIBRARY) $(LIB_OBJ)

$(EXE):$(LIBRARY) $(EXE_OBJ)
	$(CC) $(LDFLAGS) -o $@ $(EXE_OBJ) -ldxf

clean:
	rm -f *.o $(EXE) $(LIBRARY)

doc:
	doxygen

depend:
	makedepend -- $(CFLAGS) -- -Y $(SRCS)

# DO NOT DELETE

sdict.o: sdict.h
dxf_types.o: dxf_types.h
dxf.o: dxf.h util.h dxf_types.h
vdxf.o: dxf.h util.h sdict.h
