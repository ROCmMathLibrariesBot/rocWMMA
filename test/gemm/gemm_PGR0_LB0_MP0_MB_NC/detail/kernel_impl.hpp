/*******************************************************************************
 *
 * MIT License
 *
 * Copyright 2021-2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 *******************************************************************************/

#ifndef ROCWMMA_GEMM_TEST_DETAIL_KERNEL
#define ROCWMMA_GEMM_TEST_DETAIL_KERNEL

#include "device/kernel_device_func.hpp"
#include "gemm_kernel_base.hpp"

namespace rocwmma
{

    template <uint32_t BlockM,
              uint32_t BlockN,
              uint32_t BlockK,
              typename InputT,
              typename OutputT,
              typename ComputeT,
              typename LayoutA,
              typename LayoutB,
              typename LayoutC,
              typename LayoutD = LayoutC,
              uint32_t BlocksX = 1,
              uint32_t BlocksY = 1>
    struct Kernel_PGR0_LB0_MP0_MB_NC final : public GemmKernelBase<BlockM,
                                                                   BlockN,
                                                                   BlockK,
                                                                   InputT,
                                                                   OutputT,
                                                                   ComputeT,
                                                                   LayoutA,
                                                                   LayoutB,
                                                                   LayoutC,
                                                                   LayoutD>
    {
    private:
        using Base = GemmKernelBase<BlockM,
                                    BlockN,
                                    BlockK,
                                    InputT,
                                    OutputT,
                                    ComputeT,
                                    LayoutA,
                                    LayoutB,
                                    LayoutC,
                                    LayoutD>;

    public:
        Kernel_PGR0_LB0_MP0_MB_NC() {}
        ~Kernel_PGR0_LB0_MP0_MB_NC() final {}

        dim3 gridDim() const final
        {
            return dim3(ceilDiv(Base::mM, BlockM * BlocksX * Base::mTBlockX / AMDGCN_WAVE_SIZE),
                        ceilDiv(Base::mN, BlockN * BlocksY * Base::mTBlockY));
        }

        bool checkSizes() const final
        {
            return ((BlockM * BlocksX * Base::mTBlockX / AMDGCN_WAVE_SIZE) <= Base::mM)
                   && ((BlockN * BlocksY * Base::mTBlockY) <= Base::mN) && (BlockK <= Base::mK);
        }

        typename Base::KernelFunc kernelImpl() const final
        {
            return typename Base::KernelFunc(gemm_PGR0_LB0_MP0_MB_NC<BlockM,
                                                                     BlockN,
                                                                     BlockK,
                                                                     InputT,
                                                                     OutputT,
                                                                     ComputeT,
                                                                     LayoutA,
                                                                     LayoutB,
                                                                     LayoutC,
                                                                     LayoutD,
                                                                     BlocksX,
                                                                     BlocksY>);
        }

        std::ostream& printHeader(std::ostream& stream = std::cout) const final
        {
            return Base::printHeader(stream << "BlocksX, BlocksY, ");
        }

        std::ostream& printKernel(std::ostream& stream = std::cout) const final
        {
            return Base::printKernel(stream << BlocksX << ", " << BlocksY << ", ");
        }
    };

} // namespace rocwmma

#endif // ROCWMMA_GEMM_TEST_DETAIL_KERNEL
