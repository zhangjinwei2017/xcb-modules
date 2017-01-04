/*
 * Copyright (c) 2013-2017, Dalian Futures Information Technology Co., Ltd.
 *
 * Bo Wang
 * Xiaoye Meng <mengxiaoye at dce dot com dot cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <cuda_runtime.h>
#include <stdio.h>

int main(int argc, char **argv) {
	int count = 0, dev;

	cudaGetDeviceCount(&count);
	if (count == 0)
		printf("There are no available device(s) that support CUDA\n");
	else
		printf("Detected %d CUDA Capable device(s)\n", count);
	for (dev = 0; dev < count; ++dev) {
		cudaDeviceProp prop;
		int dver = 0, rver = 0;

		cudaSetDevice(dev);
		cudaGetDeviceProperties(&prop, dev);
		printf("\nDevice %d: \"%s\"\n", dev, prop.name);
		cudaDriverGetVersion(&dver);
		cudaRuntimeGetVersion(&rver);
		printf("CUDA Driver Version / Runtime Version"
			"                                               %d.%d / %d.%d\n",
			dver / 1000, dver % 100 / 10, rver / 1000, rver % 100 / 10);
		printf("CUDA Major / Minor compute capability:"
			"                                              %d.%d\n", prop.major, prop.minor);
		printf("Device has ECC support enabled:"
			"                                                     %d\n", prop.ECCEnabled);
		printf("Number of asynchronous engines:"
			"                                                     %d\n", prop.asyncEngineCount);
		printf("Device can map host memory with cudaHostAlloc/cudaHostGetDevicePointer:"
			"             %d\n", prop.canMapHostMemory);
		printf("Clock frequency in kilohertz:"
			"                                                       %d\n", prop.clockRate);
		printf("Compute mode:           "
			"                                                            %d\n", prop.computeMode);
		printf("Device can possibly execute multiple kernels concurrently:"
			"                          %d\n", prop.concurrentKernels);
		printf("Device supports caching globals in L1:"
			"                                              %d\n", prop.globalL1CacheSupported);
		printf("Device is integrated as opposed to discrete:"
			"                                        %d\n", prop.integrated);
		printf("Device is on a multi-GPU board:"
			"                                                     %d\n", prop.isMultiGpuBoard);
		printf("Specified whether there is a run time limit on kernels:"
			"                             %d\n", prop.kernelExecTimeoutEnabled);
		printf("Size of L2 cache in bytes:"
			"                                                          %d\n", prop.l2CacheSize);
		printf("Device supports caching locals in L1:"
			"                                               %d\n", prop.localL1CacheSupported);
		printf("Device supports allocating managed memory on this system:"
			"                           %d\n", prop.managedMemory);
		printf("Maximum size of each dimension of a grid:"
			"                                           (%d, %d, %d)\n",
			prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
		printf("Maximum 1D surface size: "
			"                                                           %d\n", prop.maxSurface1D);
		printf("Maximum 1D layered surface dimensions:"
			"                                              (%d, %d)\n",
			prop.maxSurface1DLayered[0], prop.maxSurface1DLayered[1]);
		printf("Maximum 2D surface dimensions:"
			"                                                      (%d, %d)\n",
			prop.maxSurface2D[0], prop.maxSurface2D[1]);
		printf("Maximum 2D layered surface dimensions:"
			"                                              (%d, %d, %d)\n",
			prop.maxSurface2DLayered[0], prop.maxSurface2DLayered[1], prop.maxSurface2DLayered[2]);
		printf("Maximum 3D surface dimensions:"
			"                                                      (%d, %d, %d)\n",
			prop.maxSurface3D[0], prop.maxSurface3D[1], prop.maxSurface3D[2]);
		printf("Maximum Cubemap surface dimensions:"
			"                                                 %d\n", prop.maxSurfaceCubemap);
		printf("Maximum Cubemap layered surface dimensions:"
			"                                         (%d, %d)\n",
			prop.maxSurfaceCubemapLayered[0], prop.maxSurfaceCubemapLayered[1]);
		printf("Maximum 1D texture size: "
			"                                                           %d\n", prop.maxTexture1D);
		printf("Maximum 1D layered texture dimensions:"
			"                                              (%d, %d)\n",
			prop.maxTexture1DLayered[0], prop.maxTexture1DLayered[1]);
		printf("Maximum size for 1D textures bound to linear memory:"
			"                                %d\n", prop.maxTexture1DLinear);
		printf("Maximum 1D mipmapped texture size:"
			"                                                  %d\n", prop.maxTexture1DMipmap);
		printf("Maximum 2D texture dimensions:"
			"                                                      (%d, %d)\n",
			prop.maxTexture2D[0], prop.maxTexture2D[1]);
		printf("Maximum 2D texture dimensions if texture gather operations have to be performed:"
			"    (%d, %d)\n", prop.maxTexture2DGather[0], prop.maxTexture2DGather[1]);
		printf("Maximum 2D layered texture dimensions:"
			"                                              (%d, %d, %d)\n",
			prop.maxTexture2DLayered[0], prop.maxTexture2DLayered[1], prop.maxTexture2DLayered[2]);
		printf("Maximum dimensions (width, height, pitch) for 2D textures bound to pitched memory:"
			"  (%d, %d, %d)\n",
			prop.maxTexture2DLinear[0], prop.maxTexture2DLinear[1], prop.maxTexture2DLinear[2]);
		printf("Maximum 2D mipmapped texture dimensions:"
			"                                            (%d, %d)\n",
			prop.maxTexture2DMipmap[0], prop.maxTexture2DMipmap[1]);
		printf("Maximum 3D texture dimensions:"
			"                                                      (%d, %d, %d)\n",
			prop.maxTexture3D[0], prop.maxTexture3D[1], prop.maxTexture3D[2]);
		printf("Maximum alternate 3D texture dimensions:"
			"                                            (%d, %d, %d)\n",
			prop.maxTexture3DAlt[0], prop.maxTexture3DAlt[1], prop.maxTexture3DAlt[2]);
		printf("Maximum Cubemap texture dimesions:"
			"                                                  %d\n", prop.maxTextureCubemap);
		printf("Maximum Cubemap layered texture dimensions:"
			"                                         (%d, %d)\n",
			prop.maxTextureCubemapLayered[0], prop.maxTextureCubemapLayered[1]);
		printf("Maximum size of each dimension of a block:"
			"                                          (%d, %d, %d)\n",
			prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
		printf("Maximum number of threads per block:"
			"                                                %d\n", prop.maxThreadsPerBlock);
		printf("Maximum resident threads per multiprocessor:"
			"                                        %d\n", prop.maxThreadsPerMultiProcessor);
		printf("Maximum pitch in bytes allowed by memory copies:"
			"                                    %lu\n", prop.memPitch);
		printf("Global memory bus width in bits:"
			"                                                    %d\n", prop.memoryBusWidth);
		printf("Peak memory clock frequency in kilohertz:"
			"                                           %d\n", prop.memoryClockRate);
		printf("Unique identifier for a group of devices on the same multi-GPU board:"
			"               %d\n", prop.multiGpuBoardGroupID);
		printf("Number of multiprocessors on device:"
			"                                                %d\n", prop.multiProcessorCount);
		printf("PCI bus ID of the device:"
			"                                                           %d\n", prop.pciBusID);
		printf("PCI device ID of the device:"
			"                                                        %d\n", prop.pciDeviceID);
		printf("PCI domain ID of the device:"
			"                                                        %d\n", prop.pciDomainID);
		printf("32-bit registers available per block:"
			"                                               %d\n", prop.regsPerBlock);
		printf("32-bit registers available per multiprocessor:"
			"                                      %d\n", prop.regsPerMultiprocessor);
		printf("Shared memory available per block in bytes:"
			"                                         %lu\n", prop.sharedMemPerBlock);
		printf("Shared memory available per multiprocessor in bytes:"
			"                                %lu\n", prop.sharedMemPerMultiprocessor);
		printf("Device supports stream priorities:    "
			"                                              %d\n", prop.streamPrioritiesSupported);
		printf("Alignment requirements for surfaces:"
			"                                                %lu\n", prop.surfaceAlignment);
		printf("1 if device is a Tesla device using TCC driver, 0 othrewise:"
			"                        %d\n", prop.tccDriver);
		printf("Alignment requirement for textures:"
			"                                                 %lu\n", prop.textureAlignment);
		printf("Pitch alignment requirement for texture references bound to pitched memory:"
			"         %lu\n", prop.texturePitchAlignment);
		printf("Constant memory available on device in bytes:"
			"                                       %lu\n", prop.totalConstMem);
		printf("Global memory available on device in bytes:"
			"                                         %lu\n", prop.totalGlobalMem);
		printf("Device shares a unified address space with the host:"
			"                                %d\n", prop.unifiedAddressing);
		printf("Warp size in threads:"
			"                                                               %d\n", prop.warpSize);
	}
	cudaDeviceReset();
	return 0;
}

