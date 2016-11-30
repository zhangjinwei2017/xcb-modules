/*
 * Copyright (c) 2013-2016, Dalian Futures Information Technology Co., Ltd.
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
		fprintf(stdout, "There are no available device(s) that support CUDA\n");
	else
		fprintf(stdout, "Detected %d CUDA Capable device(s)\n", count);
	for (dev = 0; dev < count; ++dev) {
		cudaDeviceProp prop;
		int dver = 0, rver = 0;

		cudaSetDevice(dev);
		cudaGetDeviceProperties(&prop, dev);
		fprintf(stdout, "\nDevice %d: \"%s\"\n", dev, prop.name);
		cudaDriverGetVersion(&dver);
		cudaRuntimeGetVersion(&rver);
		fprintf(stdout, "CUDA Driver Version / Runtime Version"
			"                                               %d.%d / %d.%d\n",
			dver / 1000, dver % 100 / 10, rver / 1000, rver % 100 / 10);
		fprintf(stdout, "CUDA Major / Minor compute capability:"
			"                                              %d.%d\n", prop.major, prop.minor);
		fprintf(stdout, "Device has ECC support enabled:"
			"                                                     %d\n", prop.ECCEnabled);
		fprintf(stdout, "Number of asynchronous engines:"
			"                                                     %d\n", prop.asyncEngineCount);
		fprintf(stdout, "Device can map host memory with cudaHostAlloc/cudaHostGetDevicePointer:"
			"             %d\n", prop.canMapHostMemory);
		fprintf(stdout, "Clock frequency in kilohertz:"
			"                                                       %d\n", prop.clockRate);
		fprintf(stdout, "Compute mode:           "
			"                                                            %d\n", prop.computeMode);
		fprintf(stdout, "Device can possibly execute multiple kernels concurrently:"
			"                          %d\n", prop.concurrentKernels);
		fprintf(stdout, "Device supports caching globals in L1:"
			"                                              %d\n", prop.globalL1CacheSupported);
		fprintf(stdout, "Device is integrated as opposed to discrete:"
			"                                        %d\n", prop.integrated);
		fprintf(stdout, "Device is on a multi-GPU board:"
			"                                                     %d\n", prop.isMultiGpuBoard);
		fprintf(stdout, "Specified whether there is a run time limit on kernels:"
			"                             %d\n", prop.kernelExecTimeoutEnabled);
		fprintf(stdout, "Size of L2 cache in bytes:"
			"                                                          %d\n", prop.l2CacheSize);
		fprintf(stdout, "Device supports caching locals in L1:"
			"                                               %d\n", prop.localL1CacheSupported);
		fprintf(stdout, "Device supports allocating managed memory on this system:"
			"                           %d\n", prop.managedMemory);
		fprintf(stdout, "Maximum size of each dimension of a grid:"
			"                                           (%d, %d, %d)\n",
			prop.maxGridSize[0], prop.maxGridSize[1], prop.maxGridSize[2]);
		fprintf(stdout, "Maximum 1D surface size: "
			"                                                           %d\n", prop.maxSurface1D);
		fprintf(stdout, "Maximum 1D layered surface dimensions:"
			"                                              (%d, %d)\n",
			prop.maxSurface1DLayered[0], prop.maxSurface1DLayered[1]);
		fprintf(stdout, "Maximum 2D surface dimensions:"
			"                                                      (%d, %d)\n",
			prop.maxSurface2D[0], prop.maxSurface2D[1]);
		fprintf(stdout, "Maximum 2D layered surface dimensions:"
			"                                              (%d, %d, %d)\n",
			prop.maxSurface2DLayered[0], prop.maxSurface2DLayered[1], prop.maxSurface2DLayered[2]);
		fprintf(stdout, "Maximum 3D surface dimensions:"
			"                                                      (%d, %d, %d)\n",
			prop.maxSurface3D[0], prop.maxSurface3D[1], prop.maxSurface3D[2]);
		fprintf(stdout, "Maximum Cubemap surface dimensions:"
			"                                                 %d\n", prop.maxSurfaceCubemap);
		fprintf(stdout, "Maximum Cubemap layered surface dimensions:"
			"                                         (%d, %d)\n",
			prop.maxSurfaceCubemapLayered[0], prop.maxSurfaceCubemapLayered[1]);
		fprintf(stdout, "Maximum 1D texture size: "
			"                                                           %d\n", prop.maxTexture1D);
		fprintf(stdout, "Maximum 1D layered texture dimensions:"
			"                                              (%d, %d)\n",
			prop.maxTexture1DLayered[0], prop.maxTexture1DLayered[1]);
		fprintf(stdout, "Maximum size for 1D textures bound to linear memory:"
			"                                %d\n", prop.maxTexture1DLinear);
		fprintf(stdout, "Maximum 1D mipmapped texture size:"
			"                                                  %d\n", prop.maxTexture1DMipmap);
		fprintf(stdout, "Maximum 2D texture dimensions:"
			"                                                      (%d, %d)\n",
			prop.maxTexture2D[0], prop.maxTexture2D[1]);
		fprintf(stdout, "Maximum 2D texture dimensions if texture gather operations have to be performed:"
			"    (%d, %d)\n", prop.maxTexture2DGather[0], prop.maxTexture2DGather[1]);
		fprintf(stdout, "Maximum 2D layered texture dimensions:"
			"                                              (%d, %d, %d)\n",
			prop.maxTexture2DLayered[0], prop.maxTexture2DLayered[1], prop.maxTexture2DLayered[2]);
		fprintf(stdout, "Maximum dimensions (width, height, pitch) for 2D textures bound to pitched memory:"
			"  (%d, %d, %d)\n",
			prop.maxTexture2DLinear[0], prop.maxTexture2DLinear[1], prop.maxTexture2DLinear[2]);
		fprintf(stdout, "Maximum 2D mipmapped texture dimensions:"
			"                                            (%d, %d)\n",
			prop.maxTexture2DMipmap[0], prop.maxTexture2DMipmap[1]);
		fprintf(stdout, "Maximum 3D texture dimensions:"
			"                                                      (%d, %d, %d)\n",
			prop.maxTexture3D[0], prop.maxTexture3D[1], prop.maxTexture3D[2]);
		fprintf(stdout, "Maximum alternate 3D texture dimensions:"
			"                                            (%d, %d, %d)\n",
			prop.maxTexture3DAlt[0], prop.maxTexture3DAlt[1], prop.maxTexture3DAlt[2]);
		fprintf(stdout, "Maximum Cubemap texture dimesions:"
			"                                                  %d\n", prop.maxTextureCubemap);
		fprintf(stdout, "Maximum Cubemap layered texture dimensions:"
			"                                         (%d, %d)\n",
			prop.maxTextureCubemapLayered[0], prop.maxTextureCubemapLayered[1]);
		fprintf(stdout, "Maximum size of each dimension of a block:"
			"                                          (%d, %d, %d)\n",
			prop.maxThreadsDim[0], prop.maxThreadsDim[1], prop.maxThreadsDim[2]);
		fprintf(stdout, "Maximum number of threads per block:"
			"                                                %d\n", prop.maxThreadsPerBlock);
		fprintf(stdout, "Maximum resident threads per multiprocessor:"
			"                                        %d\n", prop.maxThreadsPerMultiProcessor);
		fprintf(stdout, "Maximum pitch in bytes allowed by memory copies:"
			"                                    %lu\n", prop.memPitch);
		fprintf(stdout, "Global memory bus width in bits:"
			"                                                    %d\n", prop.memoryBusWidth);
		fprintf(stdout, "Peak memory clock frequency in kilohertz:"
			"                                           %d\n", prop.memoryClockRate);
		fprintf(stdout, "Unique identifier for a group of devices on the same multi-GPU board:"
			"               %d\n", prop.multiGpuBoardGroupID);
		fprintf(stdout, "Number of multiprocessors on device:"
			"                                                %d\n", prop.multiProcessorCount);
		fprintf(stdout, "PCI bus ID of the device:"
			"                                                           %d\n", prop.pciBusID);
		fprintf(stdout, "PCI device ID of the device:"
			"                                                        %d\n", prop.pciDeviceID);
		fprintf(stdout, "PCI domain ID of the device:"
			"                                                        %d\n", prop.pciDomainID);
		fprintf(stdout, "32-bit registers available per block:"
			"                                               %d\n", prop.regsPerBlock);
		fprintf(stdout, "32-bit registers available per multiprocessor:"
			"                                      %d\n", prop.regsPerMultiprocessor);
		fprintf(stdout, "Shared memory available per block in bytes:"
			"                                         %lu\n", prop.sharedMemPerBlock);
		fprintf(stdout, "Shared memory available per multiprocessor in bytes:"
			"                                %lu\n", prop.sharedMemPerMultiprocessor);
		fprintf(stdout, "Device supports stream priorities:    "
			"                                              %d\n", prop.streamPrioritiesSupported);
		fprintf(stdout, "Alignment requirements for surfaces:"
			"                                                %lu\n", prop.surfaceAlignment);
		fprintf(stdout, "1 if device is a Tesla device using TCC driver, 0 othrewise:"
			"                        %d\n", prop.tccDriver);
		fprintf(stdout, "Alignment requirement for textures:"
			"                                                 %lu\n", prop.textureAlignment);
		fprintf(stdout, "Pitch alignment requirement for texture references bound to pitched memory:"
			"         %lu\n", prop.texturePitchAlignment);
		fprintf(stdout, "Constant memory available on device in bytes:"
			"                                       %lu\n", prop.totalConstMem);
		fprintf(stdout, "Global memory available on device in bytes:"
			"                                         %lu\n", prop.totalGlobalMem);
		fprintf(stdout, "Device shares a unified address space with the host:"
			"                                %d\n", prop.unifiedAddressing);
		fprintf(stdout, "Warp size in threads:"
			"                                                               %d\n", prop.warpSize);
	}
	cudaDeviceReset();
	return 0;
}

