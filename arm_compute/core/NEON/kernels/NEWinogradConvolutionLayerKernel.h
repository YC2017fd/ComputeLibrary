/*
 * Copyright (c) 2017-2018 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef __ARM_COMPUTE_NEGEMMWINOGRADCONVOLUTIONLAYERKERNEL_H__
#define __ARM_COMPUTE_NEGEMMWINOGRADCONVOLUTIONLAYERKERNEL_H__

#include "arm_compute/core/NEON/INEKernel.h"
#include "arm_compute/core/NEON/kernels/convolution/common/convolution.hpp"
#include "arm_compute/core/NEON/kernels/convolution/common/tensor.hpp"
#include "arm_compute/core/NEON/kernels/convolution/winograd/batched_blocked_gemm.hpp"
#include "arm_compute/core/NEON/kernels/convolution/winograd/winograd_gemm.hpp"

namespace arm_compute
{
class ITensor;

/** Interface for the NEON kernel to perform Winograd input transform. */
template <typename T>
class INEWinogradLayerTransformInputKernel : public INEKernel
{
public:
    /** Determine how much memory (in units of TIn) to allocate for the
     * transformed input.
     *
     * @param[in] num_batches  Number of batches in the input tensor.
     * @param[in] num_channels Number of feature maps in the input tensor.
     * @param[in] num_rows     Number of rows in each feature map.
     * @param[in] num_cols     Number of columns in each feature map.
     * @param[in] same_padding Use "SAME" padding, otherwise use "VALID".
     *
     * @return Storage size (in units of TIn) required.
     */
    virtual unsigned int get_input_storage_size(int num_batches, int num_channels, int num_rows, int num_cols, bool same_padding) const = 0;

    /** Gets the stride between matrices in the input worspace
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     * @param[in] input_shape  The shape of the input tensor.
     * @param[in] padding_type The type of padding to be used.
     *
     * @return Stride expressed in bytes.
     */
    virtual int get_matrix_stride(const KernelShape &kernel_shape, const Tensor4DShape &input_shape, const PaddingType padding_type) const = 0;

    /** Configure the output transform kernel.
     *
     * @param[in]  input_nhwc    Input tensor in NHWC data layout format.
     * @param[in]  num_batches   Number of batches in input tensor.
     * @param[in]  num_rows      Number of rows in input tensor.
     * @param[in]  num_cols      Number of columns in input tensor.
     * @param[in]  num_channels  Number of channels in input tensor.
     * @param[in]  padding       Padding type.
     * @param[out] output        Base of output matrices.
     * @param[in]  matrix_stride Stride between output matrices.
     */
    virtual void configure(const ITensor *input_nhwc, const int num_batches, const int num_rows, const int num_cols, const int num_channels,
                           const PaddingType padding, ITensor *output, const int matrix_stride) = 0;

    /** Destructor */
    virtual ~INEWinogradLayerTransformInputKernel()
    {
    }
};

/** NEON kernel to perform Winograd input transform. */
template <typename T, int OutputTileRows, int OutputTileCols, int KernelRows, int KernelCols>
class NEWinogradLayerTransformInputKernel : public INEWinogradLayerTransformInputKernel<T>
{
public:
    /** Prevent instances of this class from being copied (As this class contains pointers) */
    NEWinogradLayerTransformInputKernel(const NEWinogradLayerTransformInputKernel &) = delete;
    /** Prevent instances of this class from being copied (As this class contains pointers) */
    NEWinogradLayerTransformInputKernel &operator=(const NEWinogradLayerTransformInputKernel &) = delete;
    /** Allow instances of this class to be moved */
    NEWinogradLayerTransformInputKernel(NEWinogradLayerTransformInputKernel &&) = default;
    /** Allow instances of this class to be moved */
    NEWinogradLayerTransformInputKernel &operator=(NEWinogradLayerTransformInputKernel &&) = default;
    /** Default destructor */
    ~NEWinogradLayerTransformInputKernel() = default;

    /** Determine how much memory (in units of TIn) to allocate for the
     * transformed input.
     *
     * @param[in] num_batches  Number of batches in the input tensor.
     * @param[in] num_channels Number of feature maps in the input tensor.
     * @param[in] num_rows     Number of rows in each feature map.
     * @param[in] num_cols     Number of columns in each feature map.
     * @param[in] same_padding Use "SAME" padding, otherwise use "VALID".
     *
     * @return Storage size (in units of TIn) required.
     */
    unsigned int get_input_storage_size(
        int  num_batches,
        int  num_channels,
        int  num_rows,
        int  num_cols,
        bool same_padding) const override;

    /** Gets the stride between matrices in the input worspace
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     * @param[in] input_shape  The shape of the input tensor.
     * @param[in] padding_type The type of padding to be used.
     *
     * @return Stride expressed in bytes.
     */
    int get_matrix_stride(const KernelShape &kernel_shape, const Tensor4DShape &input_shape, const PaddingType padding_type) const override;

    /** Default constructor */
    NEWinogradLayerTransformInputKernel();

    const char *name() const override
    {
        return "NEWinogradLayerTransformInputKernel";
    }

    /** Configure the output transform kernel.
     *
     * @param[in]  input_nhwc    Input tensor.  Data types supported: F32. Layout supported NHWC.
     * @param[in]  num_batches   Number of batches in input tensor.
     * @param[in]  num_rows      Number of rows in input tensor.
     * @param[in]  num_cols      Number of columns in input tensor.
     * @param[in]  num_channels  Number of channels in input tensor.
     * @param[in]  padding       Padding type.
     * @param[out] output        Base of output matrices.
     * @param[in]  matrix_stride Stride between output matrices.
     */
    void configure(
        const ITensor    *input_nhwc,
        const int         num_batches,
        const int         num_rows,
        const int         num_cols,
        const int         num_channels,
        const PaddingType padding,
        ITensor          *output,
        const int         matrix_stride) override;

    // Inherited methods overridden:
    void run(const Window &window, const ThreadInfo &info) override;

    /** Winograd base kernel */
    using WinogradBase = winograd::WinogradGEMM<OutputTileRows, OutputTileCols, KernelCols, KernelCols>;
    /** Winograd convolution kernel */
    using WinogradConv = typename WinogradBase::template Convolution<T, T>;

    /** Static function to check if given info will lead to a valid configuration of @ref NEWinogradLayerTransformInputKernel
     *
     * @param[in] input         First tensor input info. Data types supported: F32.
     * @param[in] output        Output tensor info. Data types supported: same as @p input.
     * @param[in] winograd_info Contains Winograd's information described in @ref WinogradInfo
     *
     * @return a status
     */
    static Status validate(const ITensorInfo *input, const ITensorInfo *output, const WinogradInfo &winograd_info);

private:
    using InputTransform = typename WinogradBase::template InputTransform<T>;
    const ITensor *_input_nhwc;
    int            _num_batches;   /**< Number of batches in input tensor. */
    int            _num_rows;      /**< Number of rows in input tensor. */
    int            _num_cols;      /**< Number of columns in input tensor. */
    int            _num_channels;  /**< Number of channels in input tensor. */
    PaddingType    _padding;       /**< Padding type. */
    ITensor       *_output;        /**< Base of output matrices. */
    int            _matrix_stride; /**< Stride between output matrices. */
};

/** Interface for the NEON kernel to perform Winograd output transform. */
template <typename T>
class INEWinogradLayerTransformOutputKernel : public INEKernel
{
public:
    /** Determine how much memory (in units of TOut) to allocate for the
     * (Winograd domain) output.
     *
     * @param[in] num_batches         Number of batches in the output tensor.
     * @param[in] num_rows            Number of rows in each feature map of the input tensor.
     * @param[in] num_cols            Number of columns in each feature map of the input tensor.
     * @param[in] num_output_channels Number of feature maps in the output tensor.
     * @param[in] same_padding        Use "SAME" padding, otherwise use "VALID".
     *
     * @return Storage size (in units of TOut) required.
     */
    virtual unsigned int get_output_storage_size(int num_batches, int num_rows, int num_cols, int num_output_channels, bool same_padding) const = 0;

    /** Gets the stride between matrices in the output worspace
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     * @param[in] input_shape  The shape of the input tensor.
     * @param[in] padding_type The type of padding to be used.
     *
     * @return Stride expressed in bytes.
     */
    virtual int get_matrix_stride(const KernelShape &kernel_shape, const Tensor4DShape &input_shape, const PaddingType padding_type) const = 0;

    /** Get the output shape of a convolution.
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     * @param[in] in_shape     The shape of the input tensor.
     * @param[in] padding      The type of padding to be used.
     *
     * @return Stride expressed in bytes.
     */
    virtual Tensor4DShape get_output_shape(const KernelShape &kernel_shape, const Tensor4DShape &in_shape, const PaddingType padding) const = 0;

    /** Configure the output transform kernel.
     *
     * @param[in]  biases              Pointer to the biases tensor.
     * @param[in]  output_workingspace Pointer to working space for the output tensor in the Winograd domain.
     * @param[in]  matrix_stride       Output matrix stride, can be computed with winograd::WinogradGEMM<2, 2, 3, 3>::Convolution<float, float>::get_output_matrix_stride()
     * @param[out] output_nhwc         Pointer to a tensor in NHWC data layout ordered output tensor, in the spatial domain.
     * @param[in]  num_batches         Number of batches in the input tensor.
     * @param[in]  num_rows            Number of rows in output tensor.
     * @param[in]  num_cols            Number of columns in output tensor.
     * @param[in]  num_channels        Number of feature maps in the output tensor.
     */
    virtual void configure(
        const ITensor *biases,
        const ITensor *output_workingspace,
        const int      matrix_stride,
        ITensor       *output_nhwc,
        const int      num_batches,
        const int      num_rows,
        const int      num_cols,
        const int      num_channels) = 0;

    virtual ~INEWinogradLayerTransformOutputKernel()
    {
    }
};

/** NEON kernel to perform Winograd output transform. */
template <typename T, int OutputTileRows, int OutputTileCols, int KernelRows, int KernelCols>
class NEWinogradLayerTransformOutputKernel : public INEWinogradLayerTransformOutputKernel<T>
{
public:
    const char *name() const override
    {
        return "NEWinogradLayerTransformOutputKernel";
    }
    /** Constructor */
    NEWinogradLayerTransformOutputKernel();

    /** Prevent instances of this class from being copied (As this class contains pointers) */
    NEWinogradLayerTransformOutputKernel(const NEWinogradLayerTransformOutputKernel &) = delete;
    /** Prevent instances of this class from being copied (As this class contains pointers) */
    NEWinogradLayerTransformOutputKernel &operator=(const NEWinogradLayerTransformOutputKernel &) = delete;
    /** Allow instances of this class to be moved */
    NEWinogradLayerTransformOutputKernel(NEWinogradLayerTransformOutputKernel &&) = default;
    /** Allow instances of this class to be moved */
    NEWinogradLayerTransformOutputKernel &operator=(NEWinogradLayerTransformOutputKernel &&) = default;
    /** Default destructor */
    ~NEWinogradLayerTransformOutputKernel() = default;

    // Inherited methods overridden:
    /** Determine how much memory (in units of TOut) to allocate for the
     * (Winograd domain) output.
     *
     * @param[in] num_batches         Number of batches in the output tensor.
     * @param[in] num_rows            Number of rows in each feature map of the input tensor.
     * @param[in] num_cols            Number of columns in each feature map of the input tensor.
     * @param[in] num_output_channels Number of feature maps in the output tensor.
     * @param[in] same_padding        Use "SAME" padding, otherwise use "VALID".
     *
     * @return Storage size (in units of TOut) required.
     */
    unsigned int get_output_storage_size(int num_batches, int num_rows, int num_cols, int num_output_channels, bool same_padding) const override;

    /** Gets the stride between matrices in the output worspace
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     * @param[in] input_shape  The shape of the input tensor.
     * @param[in] padding_type The type of padding to be used.
     *
     * @return Stride expressed in bytes.
     */
    int get_matrix_stride(const KernelShape &kernel_shape, const Tensor4DShape &input_shape, const PaddingType padding_type) const override;
    /** Get the output shape of a convolution.
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     * @param[in] in_shape     The shape of the input tensor.
     * @param[in] padding      The type of padding to be used.
     *
     * @return Stride expressed in bytes.
     */
    Tensor4DShape get_output_shape(const KernelShape &kernel_shape, const Tensor4DShape &in_shape, const PaddingType padding) const override;

    /** Configure the output transform kernel.
     *
     * @param[in]  biases              Pointer to the biases tensor.
     * @param[in]  output_workingspace Pointer to working space for the output tensor in the Winograd domain.
     * @param[in]  matrix_stride       Output matrix stride, can be computed with winograd::WinogradGEMM<2, 2, 3, 3>::Convolution<float, float>::get_output_matrix_stride()
     * @param[out] output_nhwc         Pointer to a tensor with NHWC data layout, in the spatial domain.
     * @param[in]  num_batches         Number of batches in the input tensor.
     * @param[in]  num_rows            Number of rows in output tensor.
     * @param[in]  num_cols            Number of columns in output tensor.
     * @param[in]  num_channels        Number of feature maps in the output tensor.
     */
    void configure(
        const ITensor *biases,
        const ITensor *output_workingspace,
        const int      matrix_stride,
        ITensor       *output_nhwc,
        const int      num_batches,
        const int      num_rows,
        const int      num_cols,
        const int      num_channels) override;

    void run(const Window &window, const ThreadInfo &info) override;

    /** Static function to check if given info will lead to a valid configuration of @ref NEWinogradLayerTransformOutputKernel
     *
     * @param[in]  input         Source tensor with shape [C, N, 16, batches] or [C, N, 36, batches]. Data types supported: F32.
     * @param[in]  bias          Biases tensor. Shared biases supported. Biases are 1D tensor with dimensions [OFM]. It can be a nullptr. Data type supported: as @p input
     * @param[out] output        Destination tensor with shape [output_convolved_dims.width, output_convolved_dims.height, C, batches]. Data type supported: same as @p input
     * @param[in]  winograd_info Contains Winograd's information described in @ref WinogradInfo
     *
     * @return a status
     */
    static Status validate(const ITensorInfo *input, const ITensorInfo *bias, const ITensorInfo *output, const WinogradInfo &winograd_info);

private:
    using WinogradBase    = winograd::WinogradGEMM<OutputTileRows, OutputTileCols, KernelRows, KernelCols>;
    using WinogradConv    = typename WinogradBase::template Convolution<T, T>;
    using OutputTransform = typename WinogradBase::template OutputTransform<T>;

    const ITensor *_biases;
    const ITensor *_output_workspace;
    int            _matrix_stride;
    int            _matrix_row_stride;
    ITensor       *_output_nhwc;
    int            _num_batches;
    int            _num_rows;
    int            _num_cols;
    int            _num_channels;
};

/** Interface for the NEON kernel to perform Winograd weights transform. */
template <typename T>
class INEWinogradLayerTransformWeightsKernel : public INEKernel
{
public:
    /** Determine how much memory (in units of T) to allocate for the
     * transformed weights.
     *
     * @param[in] num_output_channels Number of output feature maps.
     * @param[in] num_input_channels  Number of input feature maps.
     *
     * @return Storage size (in units of T) required.
     */
    virtual unsigned int get_weight_storage_size(int num_output_channels, int num_input_channels) const = 0;
    /** Gets the stride between matrices in the kernel worspace
     *
     * @param[in] kernel_shape The shape of the weights tensor.
     *
     * @return Stride expressed in bytes.
     */
    virtual int get_matrix_stride(const KernelShape &kernel_shape) const = 0;

    /** Configure the weights transform kernel.
     *
     * @param[in]  weights_hwio        Pointer to the weights tensor
     * @param[out] output              Pointer to working space for the output tensor in the Winograd domain.
     * @param[in]  matrix_stride       Stride across matrices in the output workspace.
     * @param[in]  num_output_channels Number of filters.
     * @param[in]  num_input_channels  Number of channels in each filter.
     */

    virtual void configure(const ITensor *weights_hwio, ITensor *output, const int matrix_stride, const int num_output_channels, const int num_input_channels) = 0;

    virtual ~INEWinogradLayerTransformWeightsKernel()
    {
    }
};

/** NEON kernel to perform Winograd weights transform. */
template <typename T, int OutputTileRows, int OutputTileCols, int KernelRows, int KernelCols>
class NEWinogradLayerTransformWeightsKernel final : public INEWinogradLayerTransformWeightsKernel<T>
{
public:
    /** Prevent instances of this class from being copied (As this class contains pointers) */
    NEWinogradLayerTransformWeightsKernel(const NEWinogradLayerTransformWeightsKernel &) = delete;
    /** Prevent instances of this class from being copied (As this class contains pointers) */
    NEWinogradLayerTransformWeightsKernel &operator=(const NEWinogradLayerTransformWeightsKernel &) = delete;
    /** Allow instances of this class to be moved */
    NEWinogradLayerTransformWeightsKernel(NEWinogradLayerTransformWeightsKernel &&) = default;
    /** Allow instances of this class to be moved */
    NEWinogradLayerTransformWeightsKernel &operator=(NEWinogradLayerTransformWeightsKernel &&) = default;
    /** Default destructor */
    ~NEWinogradLayerTransformWeightsKernel() = default;

    /** Default constructor. */
    NEWinogradLayerTransformWeightsKernel();
    const char *name() const override
    {
        return "NEWinogradLayerTransformWeightsKernel";
    }

    /** Static function to check if given info will lead to a valid configuration of @ref NEWinogradLayerTransformWeightsKernel
     *
     * @param[in] input         Source tensor info. The input is a 4D tensor with dimensions [kernel_x, kernel_y, IFM, OFM] (NCHW data layout).
     *                          kernel_x must be 3 and equal to kernel_y. Data types supported: F32.
     * @param[in] output        Destination tensor info. The output is a 3D tensor with dimensions [OFM, IFM, 16] or [OFM, IFM, 36]. Data type supported: same as @p input
     * @param[in] winograd_info Contains Winograd's information described in @ref WinogradInfo
     *
     * @return a status
     */
    static Status validate(const ITensorInfo *input, const ITensorInfo *output, const WinogradInfo &winograd_info);

    // Inherited methods overridden:
    void configure(const ITensor *weights_hwio, ITensor *output, const int matrix_stride, const int num_output_channels, const int num_input_channels) override;
    unsigned int get_weight_storage_size(int num_output_channels, int num_input_channels) const override;
    int get_matrix_stride(const KernelShape &kernel_shape) const override;
    void run(const Window &window, const ThreadInfo &info) override;
    bool is_parallelisable() const override;

private:
    using WinogradBase     = winograd::WinogradGEMM<OutputTileRows, OutputTileCols, KernelRows, KernelCols>;
    using WinogradConv     = typename WinogradBase::template Convolution<T, T>;
    using WeightsTransform = typename WinogradBase::template WeightsTransform<T>;

    const ITensor *_weights_hwio;
    ITensor       *_output;
    int            _matrix_stride;
    int            _num_output_channels;
    int            _num_input_channels;
};

/** NEON kernel to perform Winograd. */
template <typename TIn, typename TOut, int OutputTileRows, int OutputTileCols, int KernelRows, int KernelCols>
class NEWinogradLayerConfiguration
{
public:
    /** Winograd base kernel */
    using WinogradBase = winograd::WinogradGEMM<OutputTileRows, OutputTileCols, KernelRows, KernelCols>;
    /** Winograd convolution kernel */

    using WinogradConv = typename WinogradBase::template Convolution<TIn, TOut>;

    using TransformInputKernel   = NEWinogradLayerTransformInputKernel<TIn, OutputTileRows, OutputTileCols, KernelRows, KernelCols>;
    using TransformWeightsKernel = NEWinogradLayerTransformWeightsKernel<TIn, OutputTileRows, OutputTileCols, KernelRows, KernelCols>;
    using TransformOutputKernel  = NEWinogradLayerTransformOutputKernel<TOut, OutputTileRows, OutputTileCols, KernelRows, KernelCols>;
};

} // namespace arm_compute
#endif /*__ARM_COMPUTE_NEGEMMWINOGRADCONVOLUTIONLAYERKERNEL_H__*/
