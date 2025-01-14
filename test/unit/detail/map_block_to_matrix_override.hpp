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

#ifndef ROCWMMA_DETAIL_MAP_BLOCK_TO_MATRIX_OVERRIDE_HPP
#define ROCWMMA_DETAIL_MAP_BLOCK_TO_MATRIX_OVERRIDE_HPP

#include "device/map_block_to_matrix_override.hpp"
#include "unit_kernel_base.hpp"

namespace rocwmma
{

    template <uint32_t BlockM, uint32_t BlockN, typename DataT, typename Layout>
    struct MapBlockToMatrixOverrideKernel
        : public UnitKernelBase<BlockM, BlockN, DataT, Layout>
    {
    private:
        using Base = UnitKernelBase<BlockM, BlockN, DataT, Layout>;

    protected:
        enum : int32_t
        {
            OVERRIDE_M,
            OVERRIDE_N

        } mOverride;

    public:
        MapBlockToMatrixOverrideKernel(decltype(mOverride) overrideContext)
        : Base(), mOverride(overrideContext){};

        virtual ~MapBlockToMatrixOverrideKernel() = default;

        void setupImpl(typename Base::DataStorage::ProblemSize const& probsize) final
        {
            auto& dataInstance = Base::DataStorage::instance();
            srand((unsigned)time(0));

            // Determine override param:
            // Base::mParam1 is stored as DataT, so we must make sure we
            // can store an accurate integer representation for override.
            // must be between [0, min(maxInt, blocks - 1)] for block rw.
            auto maxInt = maxExactInteger<DataT>();
            auto blocks = mOverride == OVERRIDE_M ? 
                Base::gridDim().x * (Base::blockDim().x / AMDGCN_WAVE_SIZE) :
                Base::gridDim().y * Base::blockDim().y;
            using MaxIntT = decltype(maxInt);
            auto upperBound = std::max(
                static_cast<MaxIntT>(0), 
                std::min(maxInt, static_cast<MaxIntT>(blocks - 1)));

            // Modulate if non zero
            upperBound = upperBound ? rand() % upperBound : upperBound;
            Base::mParam1    = static_cast<DataT>(static_cast<float32_t>(upperBound));

            // Initialize matrix storage
            const int64_t sizeD = Base::mM * Base::mN;
            dataInstance->resizeStorage(probsize);

            // Initialize matrix data on host
            MatrixUtil<Layout>::fill(dataInstance->hostIn().get(), Base::mM, Base::mN);
            MatrixUtil<Layout>::fill(
                dataInstance->hostOut().get(), Base::mM, Base::mN, std::numeric_limits<DataT>::signaling_NaN());

            dataInstance->copyData(dataInstance->deviceIn(), dataInstance->hostIn(), sizeD);
            dataInstance->copyData(dataInstance->deviceOut(), dataInstance->hostOut(), sizeD);
        }

        void validateResultsImpl() final
        {
            auto& dataInstance = Base::DataStorage::instance();

            const int64_t sizeD = Base::mM * Base::mN;
            dataInstance->copyData(dataInstance->hostOut(), dataInstance->deviceOut(), sizeD);

            // Allocate host memory for reference calcs
            auto& hostInput   = dataInstance->hostIn();
            auto hostResult   = dataInstance->template allocHost<DataT>(sizeD);
                        
            /// Initialize reference for validation

            // Setup addressing for DataLayout
            auto rowMjrOffset = [](std::pair<uint32_t, uint32_t> const& matrixCoord, uint32_t ld) 
            { 
                return std::get<0>(matrixCoord) * ld + std::get<1>(matrixCoord); 
            };
            auto colMjrOffset = [](std::pair<uint32_t, uint32_t> const& matrixCoord, uint32_t ld) 
            { 
                return std::get<1>(matrixCoord) * ld + std::get<0>(matrixCoord); 
            };
            auto ld = std::is_same<Layout, row_major>::value ? Base::mN : Base::mM;
            auto arrayOffset = std::is_same<Layout, row_major>::value ? rowMjrOffset : colMjrOffset;

            // Scale from workgroup grid to wave grid
            auto waveGridDim = std::make_pair(Base::gridDim().x * (Base::blockDim().x / AMDGCN_WAVE_SIZE),
                                              Base::gridDim().y * Base::blockDim().y);

            // Calculate the expected output from the inputs
            #pragma omp parallel for
            for(auto row = 0u; row < std::get<0>(waveGridDim); row++)
            {
                #pragma omp parallel for
                for(auto col = 0u; col < std::get<1>(waveGridDim); col++)
                {
                    // Setup read / write matrix coords for block
                    auto overrideVal = static_cast<uint32_t>(static_cast<float32_t>(Base::mParam1));
                    auto readBase  = std::make_pair((mOverride == OVERRIDE_M ? overrideVal : row) * BlockM,
                                                    (mOverride == OVERRIDE_N ? overrideVal : col) * BlockN);

                    auto writeBase = std::make_pair(row * BlockM,
                                                    col * BlockN);

                    // Loop through entire read block and copy to dest block
                    for(auto rowOffset = 0u; rowOffset < BlockM; rowOffset++)
                    {
                        for(auto colOffset = 0u; colOffset < BlockN; colOffset++)
                        {
                            auto matrixRead = readBase + std::make_pair(rowOffset, colOffset);
                            auto matrixWrite = writeBase + std::make_pair(rowOffset, colOffset);

                            *(hostResult.get() + arrayOffset(matrixWrite, ld)) = 
                                *(hostInput.get() + arrayOffset(matrixRead, ld));

                        }
                    }
                }
            }
            
            double errorTolerance = 1.0;
            std::tie(Base::mValidationResult, Base::mMaxRelativeError)
                = compareEqual<DataT, DataT, Layout, Layout>(dataInstance->hostOut().get(),
                                                             hostResult.get(),
                                                             Base::mM,
                                                             Base::mN,
                                                             errorTolerance);
        }
    };

    template <uint32_t BlockM, uint32_t BlockN, typename DataT, typename Layout>
    struct MapBlockToMatrixOverrideMKernel final
        : public MapBlockToMatrixOverrideKernel<BlockM, BlockN, DataT, Layout>
    {
    private:
        using Base = MapBlockToMatrixOverrideKernel<BlockM, BlockN, DataT, Layout>;

    public:
        MapBlockToMatrixOverrideMKernel() : Base(Base::OVERRIDE_M) {};
        ~MapBlockToMatrixOverrideMKernel() final = default;

        typename Base::KernelFunc kernelImpl() const final
        {
            return
                typename Base::KernelFunc(MapBlockToMatrixOverrideM<BlockM, BlockN, DataT, Layout>);
        }
    };

    template <uint32_t BlockM, uint32_t BlockN, typename DataT, typename Layout>
    struct MapBlockToMatrixOverrideNKernel final
        : public MapBlockToMatrixOverrideKernel<BlockM, BlockN, DataT, Layout>
    {
    private:
        using Base = MapBlockToMatrixOverrideKernel<BlockM, BlockN, DataT, Layout>;

    public:
        MapBlockToMatrixOverrideNKernel() : Base(Base::OVERRIDE_N) {};
        ~MapBlockToMatrixOverrideNKernel() final = default;

        typename Base::KernelFunc kernelImpl() const final
        {
            return
                typename Base::KernelFunc(MapBlockToMatrixOverrideN<BlockM, BlockN, DataT, Layout>);
        }
    };

    // This is the GeneratorImpl class
    template<
     template <uint32_t BlockM, uint32_t BlockN, typename DataT, typename Layout>
     class OverrideClass>
    struct MapBlockToMatrixOverrideGenerator
    {
        // Indices to test parameters
        enum : uint32_t
        {
            DataT  = 0,
            BlockM = 1,
            BlockN = 2,
            Layout = 3
        };

        using ResultT = std::shared_ptr<KernelI>;

        template <typename... Ts>
        static ResultT generate(std::tuple<Ts...> testParams)
        {
            // Map GTest params to Kernel params
            using TestParamsT = std::tuple<Ts...>;
            using KernelT     = OverrideClass<
                std::tuple_element_t<BlockM, TestParamsT>::value, // BlockM
                std::tuple_element_t<BlockN, TestParamsT>::value, // BlockN
                std::tuple_element_t<DataT, TestParamsT>, // DataT
                std::tuple_element_t<Layout, TestParamsT> // Layout
                >;

            return std::make_shared<KernelT>();
        }
    };

    using MapBlockToMatrixOverrideMGenerator = MapBlockToMatrixOverrideGenerator<MapBlockToMatrixOverrideMKernel>;
    using MapBlockToMatrixOverrideNGenerator = MapBlockToMatrixOverrideGenerator<MapBlockToMatrixOverrideNKernel>;

} // namespace rocwmma

#endif // ROCWMMA_DETAIL_MAP_BLOCK_TO_MATRIX_OVERRIDE_HPP
