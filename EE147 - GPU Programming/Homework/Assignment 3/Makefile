export PATH := /usr/local/cuda-13/bin:$(PATH)
NVCC        = nvcc
ifeq (,$(shell which ncu))
NVCC_FLAGS  = -O3 
else
NVCC_FLAGS  = -O3  --std=c++03
endif
LD_FLAGS    = -lcudart 
EXE	        = sgemm-tiled
OBJ	        = main.o support.o

default: $(EXE)

main.o: main.cu kernel.cu support.h
	$(NVCC) -c -o $@ main.cu $(NVCC_FLAGS)

support.o: support.cu support.h
	$(NVCC) -c -o $@ support.cu $(NVCC_FLAGS)

$(EXE): $(OBJ)
	$(NVCC) $(OBJ) -o $(EXE) $(LD_FLAGS)

clean:
	rm -rf *.o $(EXE)
