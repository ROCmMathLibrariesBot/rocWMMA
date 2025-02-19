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
#ifndef ROCWMMA_BROADCAST_HPP
#define ROCWMMA_BROADCAST_HPP

#include "types.hpp"

namespace rocwmma
{

    // Broadcast generates vectors of desired values
    template <typename DataT, uint32_t VectorSize>
    struct Broadcast
    {
        struct Traits
        {
            using BroadcastT = VecT<DataT, VectorSize>;
        };

        __device__ static inline void exec(typename Traits::BroadcastT& data, DataT val)
        {
#pragma unroll
            for(unsigned int i = 0; i < Traits::BroadcastT::size(); i++)
            {
                data[i] = val;
            }
        }
    };

} // namespace rocwmma

#endif // ROCWMMA_BROADCAST_HPP
