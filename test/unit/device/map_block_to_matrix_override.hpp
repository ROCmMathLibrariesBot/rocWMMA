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

#ifndef ROCWMMA_DEVICE_MAP_BLOCK_TO_MATRIX_OVERRIDE_HPP
#define ROCWMMA_DEVICE_MAP_BLOCK_TO_MATRIX_OVERRIDE_HPP

#include <rocwmma/rocwmma.hpp>

namespace rocwmma
{

    template <uint32_t BlockM, uint32_t BlockN, typename DataT, typename DataLayout>
    __global__ void MapBlockToMatrixOverrideM(uint32_t     m,
                                              uint32_t     n,
                                              DataT const* in,
                                              DataT*       out,
                                              uint32_t     ld,
                                              DataT        param1,
                                              DataT        param2)
    {
        using Frag = fragment<accumulator, BlockM, BlockN, 1, DataT, DataLayout>;
        using Mapping = typename Frag::IOConfig::MappingUtil;

        // Override read M coord
        auto readCoord = Mapping::matrixCoord(
            Mapping::blockCoordM(static_cast<uint32_t>(static_cast<float32_t>(param1))));
        auto writeCoord = Mapping::matrixCoord();

        Frag f;
        load_matrix_sync(f, Mapping::dataCoord(in, readCoord, ld), ld);
        store_matrix_sync(Mapping::dataCoord(out, writeCoord, ld), f, ld);
    }

    template <uint32_t BlockM, uint32_t BlockN, typename DataT, typename DataLayout>
    __global__ void MapBlockToMatrixOverrideN(uint32_t     m,
                                              uint32_t     n,
                                              DataT const* in,
                                              DataT*       out,
                                              uint32_t     ld,
                                              DataT        param1,
                                              DataT        param2)
    {
        using Frag = fragment<accumulator, BlockM, BlockN, 1, DataT, DataLayout>;
        using Mapping = typename Frag::IOConfig::MappingUtil;

        // Override read N coord
        auto readCoord = Mapping::matrixCoord(
            Mapping::blockCoordN(static_cast<uint32_t>(static_cast<float32_t>(param1))));
        auto writeCoord = Mapping::matrixCoord();

        Frag f;
        load_matrix_sync(f, Mapping::dataCoord(in, readCoord, ld), ld);
        store_matrix_sync(Mapping::dataCoord(out, writeCoord, ld), f, ld);
    }

} // namespace rocwmma

#endif // ROCWMMA_DEVICE_MAP_BLOCK_TO_MATRIX_OVERRIDE_HPP
