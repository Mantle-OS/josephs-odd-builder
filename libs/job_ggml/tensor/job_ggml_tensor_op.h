#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <span>
#include <vector>

#include <ggml.h>

#include "job_ggml_context.h"
#include "job_ggml_enums.h"
#include "job_ggml_tensor.h"
#include "jobggml_export.h"

#ifndef GGML_N_TASKS_MAX
#define GGML_N_TASKS_MAX (-1)
#endif

namespace job::ggml {

class JobGgmlTensorOp;

using JobGgmlCustomOpFn  = std::function<void(JobGgmlTensor &dst, int ith, int nth, void *userdata)>;
using JobGgmlCustom1OpFn = std::function<void(JobGgmlTensor &dst, const JobGgmlTensor &a, int ith, int nth, void *userdata)>;
using JobGgmlCustom2OpFn = std::function<void(JobGgmlTensor &dst, const JobGgmlTensor &a, const JobGgmlTensor &b, int ith, int nth, void *userdata)>;
using JobGgmlCustom3OpFn = std::function<void(JobGgmlTensor &dst, const JobGgmlTensor &a, const JobGgmlTensor &b, const JobGgmlTensor &c, int ith, int nth, void *userdata)>;

struct JobGgmlCustomOpPayload  { JobGgmlCustomOpFn  func; void *userdata{nullptr}; };
struct JobGgmlCustom1OpPayload { JobGgmlCustom1OpFn func; void *userdata{nullptr}; };
struct JobGgmlCustom2OpPayload { JobGgmlCustom2OpFn func; void *userdata{nullptr}; };
struct JobGgmlCustom3OpPayload { JobGgmlCustom3OpFn func; void *userdata{nullptr}; };

class JOBGGML_EXPORT JobGgmlTensorOp : public JobGgmlTensor
{
public:
    using Ptr  = std::shared_ptr<JobGgmlTensorOp>;
    using WPtr = std::weak_ptr<JobGgmlTensorOp>;
    using UPtr = std::unique_ptr<JobGgmlTensorOp>;

    explicit JobGgmlTensorOp(struct ggml_tensor *tensor, JobGgmlContext *context) :
        JobGgmlTensor{tensor},
        m_ctx{context}
    {
        if (!m_ctx || !m_ctx->isValid())
            throw std::invalid_argument{ "JobGgmlTensorOp requires a valid JobGgmlContext" };

        if (!isValid())
            throw std::invalid_argument{ "JobGgmlTensorOp requires a valid GGML tensor" };
    }
    ~JobGgmlTensorOp() = default;

    [[nodiscard]] static Ptr  createShared(struct ggml_tensor *tensor, JobGgmlContext *context) { return std::make_shared<JobGgmlTensorOp>(tensor, context); }
    [[nodiscard]] static UPtr createUniq(struct ggml_tensor *tensor, JobGgmlContext *context)   { return std::make_unique<JobGgmlTensorOp>(tensor, context); }

    JobGgmlTensorOp(const JobGgmlTensorOp &) = delete;
    JobGgmlTensorOp &operator=(const JobGgmlTensorOp &) = delete;
    JobGgmlTensorOp(JobGgmlTensorOp &&) = delete;
    JobGgmlTensorOp &operator=(JobGgmlTensorOp &&) = delete;

    [[nodiscard]] JobGgmlContext *context() noexcept { return m_ctx; }
    [[nodiscard]] const JobGgmlContext *context() const noexcept { return m_ctx; }

    // Callback's
    [[nodiscard]] UPtr mapCustom1(JobGgmlCustom1OpFn func, int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustom1OpPayload>(
            JobGgmlCustom1OpPayload{std::move(func), userdata}
        );
        return operate<&ggml_map_custom1>(callBouncer1, n_tasks, payload);
    }
    // START FIXME
    [[nodiscard]] UPtr mapCustom1Inplace(JobGgmlCustom1OpFn func, int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustom1OpPayload>(JobGgmlCustom1OpPayload{std::move(func), userdata});
        return operate<&ggml_map_custom1_inplace>(callBouncer1, n_tasks, payload);
    }

    template<typename B>
    [[nodiscard]] UPtr mapCustom2(B &&b, JobGgmlCustom2OpFn func, int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustom2OpPayload>(JobGgmlCustom2OpPayload{std::move(func), userdata});
        return operate<&ggml_map_custom2>(std::forward<B>(b), callBouncer2, n_tasks, payload);
    }

    template<typename B>
    [[nodiscard]] UPtr mapCustom2Inplace(B &&b, JobGgmlCustom2OpFn func, int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustom2OpPayload>(JobGgmlCustom2OpPayload{std::move(func), userdata});
        return operate<&ggml_map_custom2_inplace>(std::forward<B>(b), callBouncer2, n_tasks, payload);
    }

    template<typename B, typename C>
    [[nodiscard]] UPtr mapCustom3(B &&b, C &&c, JobGgmlCustom3OpFn func, int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustom3OpPayload>(JobGgmlCustom3OpPayload{std::move(func), userdata});
        return operate<&ggml_map_custom3>(std::forward<B>(b), std::forward<C>(c), callBouncer3, n_tasks, payload);
    }

    template<typename B, typename C>
    [[nodiscard]] UPtr mapCustom3Inplace(B &&b, C &&c, JobGgmlCustom3OpFn func, int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustom3OpPayload>(JobGgmlCustom3OpPayload{std::move(func), userdata});
        return operate<&ggml_map_custom3_inplace>(std::forward<B>(b), std::forward<C>(c), callBouncer3, n_tasks, payload);
    }

    [[nodiscard]] UPtr custom4d(JobGgmlType type, int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3,
                                struct ggml_tensor **args, int n_args, JobGgmlCustomOpFn func,
                                int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustomOpPayload>(JobGgmlCustomOpPayload{std::move(func), userdata});
        return rawOperate<&ggml_custom_4d>(type, ne0, ne1, ne2, ne3, args, n_args, callBouncer, n_tasks, payload);
    }

    [[nodiscard]] UPtr customInplace(struct ggml_tensor **args, int n_args, JobGgmlCustomOpFn func,
                                     int n_tasks, void *userdata = nullptr)
    {
        auto *payload = m_ctx->createPayload<JobGgmlCustomOpPayload>(JobGgmlCustomOpPayload{std::move(func), userdata});
        return operate<&ggml_custom_inplace>(args, n_args, callBouncer, n_tasks, payload);
    }
    [[nodiscard]] UPtr custom4d(JobGgmlType type, int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3,
                                std::span<JobGgmlTensor *const> args, JobGgmlCustomOpFn func,
                                int n_tasks, void *userdata = nullptr)
    {
        std::vector<struct ggml_tensor *> nativeArgs;
        nativeArgs.reserve(args.size());

        for (JobGgmlTensor *arg : args) {
            if (!arg || !arg->isValid())
                throw std::invalid_argument{"custom4d requires valid JobGgmlTensor arguments"};

            nativeArgs.push_back(arg->tensor());
        }

        return custom4d(type, ne0, ne1, ne2, ne3,
                        nativeArgs.data(), static_cast<int>(nativeArgs.size()),
                        std::move(func), n_tasks, userdata);
    }

    [[nodiscard]] UPtr customInplace(std::span<JobGgmlTensor *const> args, JobGgmlCustomOpFn func,
                                     int n_tasks, void *userdata = nullptr)
    {
        std::vector<struct ggml_tensor *> nativeArgs;
        nativeArgs.reserve(args.size());

        for (JobGgmlTensor *arg : args) {
            if (!arg || !arg->isValid())
                throw std::invalid_argument{"customInplace requires valid JobGgmlTensor arguments"};

            nativeArgs.push_back(arg->tensor());
        }

        return customInplace(nativeArgs.data(), static_cast<int>(nativeArgs.size()),
                             std::move(func), n_tasks, userdata);
    }
    // Unary Operations
    [[nodiscard]] UPtr dup() { return operate<&ggml_dup>(); }
    [[nodiscard]] UPtr dupInplace() { return operate<&ggml_dup_inplace>(); }
    [[nodiscard]] UPtr sqr() { return operate<&ggml_sqr>(); }
    [[nodiscard]] UPtr sqrInplace() { return operate<&ggml_sqr_inplace>(); }
    [[nodiscard]] UPtr sqrt() { return operate<&ggml_sqrt>(); }
    [[nodiscard]] UPtr sqrtInplace() { return operate<&ggml_sqrt_inplace>(); }
    [[nodiscard]] UPtr log() { return operate<&ggml_log>(); }
    [[nodiscard]] UPtr logInplace() { return operate<&ggml_log_inplace>(); }
    [[nodiscard]] UPtr expm1() { return operate<&ggml_expm1>(); }
    [[nodiscard]] UPtr expm1Inplace() { return operate<&ggml_expm1_inplace>(); }
    [[nodiscard]] UPtr softplus() { return operate<&ggml_softplus>(); }
    [[nodiscard]] UPtr softplusInplace() { return operate<&ggml_softplus_inplace>(); }
    [[nodiscard]] UPtr sin() { return operate<&ggml_sin>(); }
    [[nodiscard]] UPtr sinInplace() { return operate<&ggml_sin_inplace>(); }
    [[nodiscard]] UPtr cos() { return operate<&ggml_cos>(); }
    [[nodiscard]] UPtr cosInplace() { return operate<&ggml_cos_inplace>(); }
    [[nodiscard]] UPtr sum() { return operate<&ggml_sum>(); }
    [[nodiscard]] UPtr sumRows() { return operate<&ggml_sum_rows>(); }
    [[nodiscard]] UPtr cumsum() { return operate<&ggml_cumsum>(); }
    [[nodiscard]] UPtr mean() { return operate<&ggml_mean>(); }
    [[nodiscard]] UPtr argmax() { return operate<&ggml_argmax>(); }
    [[nodiscard]] UPtr abs() { return operate<&ggml_abs>(); }
    [[nodiscard]] UPtr absInplace() { return operate<&ggml_abs_inplace>(); }
    [[nodiscard]] UPtr sgn() { return operate<&ggml_sgn>(); }
    [[nodiscard]] UPtr sgnInplace() { return operate<&ggml_sgn_inplace>(); }
    [[nodiscard]] UPtr neg() { return operate<&ggml_neg>(); }
    [[nodiscard]] UPtr negInplace() { return operate<&ggml_neg_inplace>(); }
    [[nodiscard]] UPtr step() { return operate<&ggml_step>(); }
    [[nodiscard]] UPtr stepInplace() { return operate<&ggml_step_inplace>(); }
    [[nodiscard]] UPtr tanh() { return operate<&ggml_tanh>(); }
    [[nodiscard]] UPtr tanhInplace() { return operate<&ggml_tanh_inplace>(); }
    [[nodiscard]] UPtr elu() { return operate<&ggml_elu>(); }
    [[nodiscard]] UPtr eluInplace() { return operate<&ggml_elu_inplace>(); }
    [[nodiscard]] UPtr relu() { return operate<&ggml_relu>(); }
    [[nodiscard]] UPtr reluInplace() { return operate<&ggml_relu_inplace>(); }
    [[nodiscard]] UPtr sigmoid() { return operate<&ggml_sigmoid>(); }
    [[nodiscard]] UPtr sigmoidInplace() { return operate<&ggml_sigmoid_inplace>(); }
    [[nodiscard]] UPtr gelu() { return operate<&ggml_gelu>(); }
    [[nodiscard]] UPtr geluInplace() { return operate<&ggml_gelu_inplace>(); }
    [[nodiscard]] UPtr geluErf() { return operate<&ggml_gelu_erf>(); }
    [[nodiscard]] UPtr geluErfInplace() { return operate<&ggml_gelu_erf_inplace>(); }
    [[nodiscard]] UPtr geluQuick() { return operate<&ggml_gelu_quick>(); }
    [[nodiscard]] UPtr geluQuickInplace() { return operate<&ggml_gelu_quick_inplace>(); }
    [[nodiscard]] UPtr silu() { return operate<&ggml_silu>(); }
    [[nodiscard]] UPtr siluInplace() { return operate<&ggml_silu_inplace>(); }
    [[nodiscard]] UPtr hardswish() { return operate<&ggml_hardswish>(); }
    [[nodiscard]] UPtr hardsigmoid() { return operate<&ggml_hardsigmoid>(); }
    [[nodiscard]] UPtr exp() { return operate<&ggml_exp>(); }
    [[nodiscard]] UPtr expInplace() { return operate<&ggml_exp_inplace>(); }
    [[nodiscard]] UPtr floor() { return operate<&ggml_floor>(); }
    [[nodiscard]] UPtr floorInplace() { return operate<&ggml_floor_inplace>(); }
    [[nodiscard]] UPtr ceil() { return operate<&ggml_ceil>(); }
    [[nodiscard]] UPtr ceilInplace() { return operate<&ggml_ceil_inplace>(); }
    [[nodiscard]] UPtr round() { return operate<&ggml_round>(); }
    [[nodiscard]] UPtr roundInplace() { return operate<&ggml_round_inplace>(); }
    [[nodiscard]] UPtr trunc() { return operate<&ggml_trunc>(); }
    [[nodiscard]] UPtr truncInplace() { return operate<&ggml_trunc_inplace>(); }
    [[nodiscard]] UPtr reglu() { return operate<&ggml_reglu>(); }
    [[nodiscard]] UPtr regluSwapped() { return operate<&ggml_reglu_swapped>(); }
    [[nodiscard]] UPtr geglu() { return operate<&ggml_geglu>(); }
    [[nodiscard]] UPtr gegluSwapped() { return operate<&ggml_geglu_swapped>(); }
    [[nodiscard]] UPtr swiglu() { return operate<&ggml_swiglu>(); }
    [[nodiscard]] UPtr swigluSwapped() { return operate<&ggml_swiglu_swapped>(); }
    [[nodiscard]] UPtr gegluErf() { return operate<&ggml_geglu_erf>(); }
    [[nodiscard]] UPtr gegluErfSwapped() { return operate<&ggml_geglu_erf_swapped>(); }
    [[nodiscard]] UPtr gegluQuick() { return operate<&ggml_geglu_quick>(); }
    [[nodiscard]] UPtr gegluQuickSwapped() { return operate<&ggml_geglu_quick_swapped>(); }
    [[nodiscard]] UPtr cont() { return operate<&ggml_cont>(); }
    [[nodiscard]] UPtr transpose() { return operate<&ggml_transpose>(); }
    [[nodiscard]] UPtr diag() { return operate<&ggml_diag>(); }
    [[nodiscard]] UPtr softMax() { return operate<&ggml_soft_max>(); }
    [[nodiscard]] UPtr softMaxInplace() { return operate<&ggml_soft_max_inplace>(); }

    // Binary & Multi Tensor Operations
    template <typename B> [[nodiscard]] UPtr add(B&& b) { return operate<&ggml_add>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr addInplace(B&& b) { return operate<&ggml_add_inplace>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr addCast(B&& b, JobGgmlType type) { return operate<&ggml_add_cast>(std::forward<B>(b), type); }
    template <typename B, typename I> [[nodiscard]] UPtr addId(B&& b, I&& ids) { return operate<&ggml_add_id>(std::forward<B>(b), std::forward<I>(ids)); }
    template <typename B> [[nodiscard]] UPtr sub(B&& b) { return operate<&ggml_sub>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr subInplace(B&& b) { return operate<&ggml_sub_inplace>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr mul(B&& b) { return operate<&ggml_mul>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr mulInplace(B&& b) { return operate<&ggml_mul_inplace>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr div(B&& b) { return operate<&ggml_div>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr divInplace(B&& b) { return operate<&ggml_div_inplace>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr countEqual(B&& b) { return operate<&ggml_count_equal>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr repeat(B&& b) { return operate<&ggml_repeat>(std::forward<B>(b)); }
    [[nodiscard]] UPtr repeat4d(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3) { return operate<&ggml_repeat_4d>(ne0, ne1, ne2, ne3); }
    template <typename B> [[nodiscard]] UPtr repeatBack(B&& b) { return operate<&ggml_repeat_back>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr concat(B&& b, int dim) { return operate<&ggml_concat>(std::forward<B>(b), dim); }
    template <typename B> [[nodiscard]] UPtr siluBack(B&& b) { return operate<&ggml_silu_back>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr gluSplit(B&& b, JobGgmlGluOp op) { return operate<&ggml_glu_split>(std::forward<B>(b), op); }
    template <typename B> [[nodiscard]] UPtr regluSplit(B&& b) { return operate<&ggml_reglu_split>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr gegluSplit(B&& b) { return operate<&ggml_geglu_split>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr swigluSplit(B&& b) { return operate<&ggml_swiglu_split>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr gegluErfSplit(B&& b) { return operate<&ggml_geglu_erf_split>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr gegluQuickSplit(B&& b) { return operate<&ggml_geglu_quick_split>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr swigluOai(B&& b, float alpha, float limit) { return operate<&ggml_swiglu_oai>(std::forward<B>(b), alpha, limit); }
    template <typename B> [[nodiscard]] UPtr mulMat(B&& b) { return operate<&ggml_mul_mat>(std::forward<B>(b)); }
    template <typename B, typename I> [[nodiscard]] UPtr mulMatId(B&& as, I&& ids) { return operate<&ggml_mul_mat_id>(std::forward<B>(as), std::forward<I>(ids)); }
    template <typename B> [[nodiscard]] UPtr outProd(B&& b) { return operate<&ggml_out_prod>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr cpy(B&& b) { return operate<&ggml_cpy>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr reshape(B&& b) { return operate<&ggml_reshape>(std::forward<B>(b)); }
    template <typename B> [[nodiscard]] UPtr getRows(B&& b) { return operate<&ggml_get_rows>(std::forward<B>(b)); }
    template <typename B, typename C> [[nodiscard]] UPtr getRowsBack(B&& b, C&& c) { return operate<&ggml_get_rows_back>(std::forward<B>(b), std::forward<C>(c)); }
    template <typename B, typename C> [[nodiscard]] UPtr setRows(B&& b, C&& c) { return operate<&ggml_set_rows>(std::forward<B>(b), std::forward<C>(c)); }

    // Parameterized Transformations & Views
    template <typename B> [[nodiscard]] UPtr acc(B&& b, size_t nb1, size_t nb2, size_t nb3, size_t offset) { return operate<&ggml_acc>(std::forward<B>(b), nb1, nb2, nb3, offset); }
    template <typename B> [[nodiscard]] UPtr accInplace(B&& b, size_t nb1, size_t nb2, size_t nb3, size_t offset) { return operate<&ggml_acc_inplace>(std::forward<B>(b), nb1, nb2, nb3, offset); }
    [[nodiscard]] UPtr leakyRelu(float negative_slope, bool inplace) { return operate<&ggml_leaky_relu>(negative_slope, inplace); }
    [[nodiscard]] UPtr xielu(float alpha_n, float alpha_p, float beta, float eps) { return operate<&ggml_xielu>(alpha_n, alpha_p, beta, eps); }
    [[nodiscard]] UPtr glu(JobGgmlGluOp op, bool swapped) { return operate<&ggml_glu>(op, swapped); }
    [[nodiscard]] UPtr norm(float eps) { return operate<&ggml_norm>(eps); }
    [[nodiscard]] UPtr normInplace(float eps) { return operate<&ggml_norm_inplace>(eps); }
    [[nodiscard]] UPtr rmsNorm(float eps) { return operate<&ggml_rms_norm>(eps); }
    [[nodiscard]] UPtr rmsNormInplace(float eps) { return operate<&ggml_rms_norm_inplace>(eps); }
    [[nodiscard]] UPtr groupNorm(int n_groups, float eps) { return operate<&ggml_group_norm>(n_groups, eps); }
    [[nodiscard]] UPtr groupNormInplace(int n_groups, float eps) { return operate<&ggml_group_norm_inplace>(n_groups, eps); }
    [[nodiscard]] UPtr l2Norm(float eps) { return operate<&ggml_l2_norm>(eps); }
    [[nodiscard]] UPtr l2NormInplace(float eps) { return operate<&ggml_l2_norm_inplace>(eps); }
    template <typename B> [[nodiscard]] UPtr rmsNormBack(B&& b, float eps) { return operate<&ggml_rms_norm_back>(std::forward<B>(b), eps); }

    void mulMatSetPrec(JobGgmlPrecision prec) { ggml_mul_mat_set_prec(tensor(), static_cast<enum ggml_prec>(prec)); }
    void mulMatSetHint(JobGgmlOpHint hint) { ggml_mul_mat_set_hint(tensor(), static_cast<enum ggml_op_hint>(hint)); }

    [[nodiscard]] UPtr scale(float s) { return operate<&ggml_scale>(s); }
    [[nodiscard]] UPtr scaleInplace(float s) { return operate<&ggml_scale_inplace>(s); }
    [[nodiscard]] UPtr scaleBias(float s, float b) { return operate<&ggml_scale_bias>(s, b); }
    [[nodiscard]] UPtr scaleBiasInplace(float s, float b) { return operate<&ggml_scale_bias_inplace>(s, b); }

    template <typename B> [[nodiscard]] UPtr set(B&& b, size_t nb1, size_t nb2, size_t nb3, size_t offset) { return operate<&ggml_set>(std::forward<B>(b), nb1, nb2, nb3, offset); }
    template <typename B> [[nodiscard]] UPtr setInplace(B&& b, size_t nb1, size_t nb2, size_t nb3, size_t offset) { return operate<&ggml_set_inplace>(std::forward<B>(b), nb1, nb2, nb3, offset); }
    template <typename B> [[nodiscard]] UPtr set1d(B&& b, size_t offset) { return operate<&ggml_set_1d>(std::forward<B>(b), offset); }
    template <typename B> [[nodiscard]] UPtr set1dInplace(B&& b, size_t offset) { return operate<&ggml_set_1d_inplace>(std::forward<B>(b), offset); }
    template <typename B> [[nodiscard]] UPtr set2d(B&& b, size_t nb1, size_t offset) { return operate<&ggml_set_2d>(std::forward<B>(b), nb1, offset); }
    template <typename B> [[nodiscard]] UPtr set2dInplace(B&& b, size_t nb1, size_t offset) { return operate<&ggml_set_2d_inplace>(std::forward<B>(b), nb1, offset); }

    [[nodiscard]] UPtr cast(JobGgmlType type) { return operate<&ggml_cast>(type); }
    [[nodiscard]] UPtr cont1d(int64_t ne0) { return operate<&ggml_cont_1d>(ne0); }
    [[nodiscard]] UPtr cont2d(int64_t ne0, int64_t ne1) { return operate<&ggml_cont_2d>(ne0, ne1); }
    [[nodiscard]] UPtr cont3d(int64_t ne0, int64_t ne1, int64_t ne2) { return operate<&ggml_cont_3d>(ne0, ne1, ne2); }
    [[nodiscard]] UPtr cont4d(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3) { return operate<&ggml_cont_4d>(ne0, ne1, ne2, ne3); }

    [[nodiscard]] UPtr reshape1d(int64_t ne0) { return operate<&ggml_reshape_1d>(ne0); }
    [[nodiscard]] UPtr reshape2d(int64_t ne0, int64_t ne1) { return operate<&ggml_reshape_2d>(ne0, ne1); }
    [[nodiscard]] UPtr reshape3d(int64_t ne0, int64_t ne1, int64_t ne2) { return operate<&ggml_reshape_3d>(ne0, ne1, ne2); }
    [[nodiscard]] UPtr reshape4d(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3) { return operate<&ggml_reshape_4d>(ne0, ne1, ne2, ne3); }

    [[nodiscard]] UPtr view1d(int64_t ne0, size_t offset) { return operate<&ggml_view_1d>(ne0, offset); }
    [[nodiscard]] UPtr view2d(int64_t ne0, int64_t ne1, size_t nb1, size_t offset) { return operate<&ggml_view_2d>(ne0, ne1, nb1, offset); }
    [[nodiscard]] UPtr view3d(int64_t ne0, int64_t ne1, int64_t ne2, size_t nb1, size_t nb2, size_t offset) { return operate<&ggml_view_3d>(ne0, ne1, ne2, nb1, nb2, offset); }
    [[nodiscard]] UPtr view4d(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3, size_t nb1, size_t nb2, size_t nb3, size_t offset) { return operate<&ggml_view_4d>(ne0, ne1, ne2, ne3, nb1, nb2, nb3, offset); }

    [[nodiscard]] UPtr permute(int axis0, int axis1, int axis2, int axis3) { return operate<&ggml_permute>(axis0, axis1, axis2, axis3); }

    [[nodiscard]] UPtr diagMaskInf(int n_past) { return operate<&ggml_diag_mask_inf>(n_past); }
    [[nodiscard]] UPtr diagMaskInfInplace(int n_past) { return operate<&ggml_diag_mask_inf_inplace>(n_past); }
    [[nodiscard]] UPtr diagMaskZero(int n_past) { return operate<&ggml_diag_mask_zero>(n_past); }
    [[nodiscard]] UPtr diagMaskZeroInplace(int n_past) { return operate<&ggml_diag_mask_zero_inplace>(n_past); }

    template <typename M> [[nodiscard]] UPtr softMaxExt(M&& mask, float scale, float max_bias) { return operate<&ggml_soft_max_ext>(std::forward<M>(mask), scale, max_bias); }
    template <typename M> [[nodiscard]] UPtr softMaxExtInplace(M&& mask, float scale, float max_bias) { return operate<&ggml_soft_max_ext_inplace>(std::forward<M>(mask), scale, max_bias); }
    template <typename S> void softMaxAddSinks(S&& sinks) { ggml_soft_max_add_sinks(tensor(), nativeArg(std::forward<S>(sinks))); }
    template <typename B> [[nodiscard]] UPtr softMaxExtBack(B&& b, float scale, float max_bias) { return operate<&ggml_soft_max_ext_back>(std::forward<B>(b), scale, max_bias); }
    template <typename B> [[nodiscard]] UPtr softMaxExtBackInplace(B&& b, float scale, float max_bias) { return operate<&ggml_soft_max_ext_back_inplace>(std::forward<B>(b), scale, max_bias); }

    // RoPE, Attention, Convolutions, Custom Ops, Etc
    template <typename... Args> [[nodiscard]] UPtr rope(Args&&... args) { return operate<&ggml_rope>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeInplace(Args&&... args) { return operate<&ggml_rope_inplace>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeExt(Args&&... args) { return operate<&ggml_rope_ext>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeMulti(Args&&... args) { return operate<&ggml_rope_multi>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeExtInplace(Args&&... args) { return operate<&ggml_rope_ext_inplace>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeMultiInplace(Args&&... args) { return operate<&ggml_rope_multi_inplace>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeExtBack(Args&&... args) { return operate<&ggml_rope_ext_back>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ropeMultiBack(Args&&... args) { return operate<&ggml_rope_multi_back>(std::forward<Args>(args)...); }

    [[nodiscard]] UPtr clamp(float minVal, float maxVal) { return operate<&ggml_clamp>(minVal, maxVal); }
    template <typename... Args> [[nodiscard]] UPtr im2col(Args&&... args) { return operate<&ggml_im2col>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr im2colBack(Args&&... args) { return operate<&ggml_im2col_back>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr col2im1d(Args&&... args) { return operate<&ggml_col2im_1d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv1d(Args&&... args) { return operate<&ggml_conv_1d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv1dPh(Args&&... args) { return operate<&ggml_conv_1d_ph>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv1dDw(Args&&... args) { return operate<&ggml_conv_1d_dw>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv1dDwPh(Args&&... args) { return operate<&ggml_conv_1d_dw_ph>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr convTranspose1d(Args&&... args) { return operate<&ggml_conv_transpose_1d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv2d(Args&&... args) { return operate<&ggml_conv_2d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr im2col3d(Args&&... args) { return operate<&ggml_im2col_3d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv3d(Args&&... args) { return operate<&ggml_conv_3d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv2dSkP0(Args&&... args) { return operate<&ggml_conv_2d_sk_p0>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv2dS1Ph(Args&&... args) { return operate<&ggml_conv_2d_s1_ph>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv2dDw(Args&&... args) { return operate<&ggml_conv_2d_dw>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv2dDwDirect(Args&&... args) { return operate<&ggml_conv_2d_dw_direct>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr convTranspose2dP0(Args&&... args) { return operate<&ggml_conv_transpose_2d_p0>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv2dDirect(Args&&... args) { return operate<&ggml_conv_2d_direct>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr conv3dDirect(Args&&... args) { return operate<&ggml_conv_3d_direct>(std::forward<Args>(args)...); }

    template <typename... Args> [[nodiscard]] UPtr pool1d(Args&&... args) { return operate<&ggml_pool_1d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr pool2d(Args&&... args) { return operate<&ggml_pool_2d>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr pool2dBack(Args&&... args) { return operate<&ggml_pool_2d_back>(std::forward<Args>(args)...); }

    [[nodiscard]] UPtr upscale(int scale_factor, JobGgmlScaleMode mode) { return operate<&ggml_upscale>(scale_factor, mode); }
    [[nodiscard]] UPtr interpolate(int64_t ne0, int64_t ne1, int64_t ne2, int64_t ne3, uint32_t mode) { return operate<&ggml_interpolate>(ne0, ne1, ne2, ne3, mode); }
    [[nodiscard]] UPtr pad(int p0, int p1, int p2, int p3) { return operate<&ggml_pad>(p0, p1, p2, p3); }
    [[nodiscard]] UPtr padCircular(int p0, int p1, int p2, int p3) { return operate<&ggml_pad_circular>(p0, p1, p2, p3); }
    [[nodiscard]] UPtr padExt(int lp0, int rp0, int lp1, int rp1, int lp2, int rp2, int lp3, int rp3) { return operate<&ggml_pad_ext>(lp0, rp0, lp1, rp1, lp2, rp2, lp3, rp3); }
    [[nodiscard]] UPtr padExtCircular(int lp0, int rp0, int lp1, int rp1, int lp2, int rp2, int lp3, int rp3) { return operate<&ggml_pad_ext_circular>(lp0, rp0, lp1, rp1, lp2, rp2, lp3, rp3); }
    [[nodiscard]] UPtr padReflect1d(int p0, int p1) { return operate<&ggml_pad_reflect_1d>(p0, p1); }
    [[nodiscard]] UPtr roll(int shift0, int shift1, int shift2, int shift3) { return operate<&ggml_roll>(shift0, shift1, shift2, shift3); }
    [[nodiscard]] UPtr tri(JobGgmlTriType type) { return operate<&ggml_tri>(type); }
    [[nodiscard]] UPtr fill(float c) { return operate<&ggml_fill>(c); }
    [[nodiscard]] UPtr fillInplace(float c) { return operate<&ggml_fill_inplace>(c); }
    [[nodiscard]] UPtr timestepEmbedding(int dim, int max_period) { return operate<&ggml_timestep_embedding>(dim, max_period); }

    [[nodiscard]] UPtr argsort(JobGgmlSortOrder order) { return operate<&ggml_argsort>(order); }
    [[nodiscard]] UPtr argsortTopK(int k) { return operate<&ggml_argsort_top_k>(k); }
    [[nodiscard]] UPtr topK(int k) { return operate<&ggml_top_k>(k); }

    [[nodiscard]] UPtr arange(float start, float stop, float step) { return rawOperate<&ggml_arange>(start, stop, step); }

    template <typename... Args> [[nodiscard]] UPtr flashAttnExt(Args&&... args) { return operate<&ggml_flash_attn_ext>(std::forward<Args>(args)...); }
    void flashAttnExtSetPrec(JobGgmlPrecision prec) { ggml_flash_attn_ext_set_prec(tensor(), static_cast<enum ggml_prec>(prec)); }
    [[nodiscard]] JobGgmlPrecision flashAttnExtGetPrec() const { return static_cast<JobGgmlPrecision>(ggml_flash_attn_ext_get_prec(tensor())); }
    template <typename S> void flashAttnExtAddSinks(S&& sinks) { ggml_flash_attn_ext_add_sinks(tensor(), nativeArg(std::forward<S>(sinks))); }
    template <typename... Args> [[nodiscard]] UPtr flashAttnBack(Args&&... args) { return operate<&ggml_flash_attn_back>(std::forward<Args>(args)...); }

    template <typename... Args> [[nodiscard]] UPtr ssmConv(Args&&... args) { return operate<&ggml_ssm_conv>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr ssmScan(Args&&... args) { return operate<&ggml_ssm_scan>(std::forward<Args>(args)...); }
    [[nodiscard]] UPtr winPart(int w) { return operate<&ggml_win_part>(w); }
    [[nodiscard]] UPtr winUnpart(int w0, int h0, int w) { return operate<&ggml_win_unpart>(w0, h0, w); }
    [[nodiscard]] UPtr unary(JobGgmlUnaryOp op) { return operate<&ggml_unary>(op); }
    [[nodiscard]] UPtr unaryInplace(JobGgmlUnaryOp op) { return operate<&ggml_unary_inplace>(op); }
    [[nodiscard]] UPtr getRelPos(int qh, int kh) { return operate<&ggml_get_rel_pos>(qh, kh); }
    template <typename PW, typename PH> [[nodiscard]] UPtr addRelPos(PW&& pw, PH&& ph) { return operate<&ggml_add_rel_pos>(std::forward<PW>(pw), std::forward<PH>(ph)); }
    template <typename PW, typename PH> [[nodiscard]] UPtr addRelPosInplace(PW&& pw, PH&& ph) { return operate<&ggml_add_rel_pos_inplace>(std::forward<PW>(pw), std::forward<PH>(ph)); }

    template <typename... Args> [[nodiscard]] UPtr rwkvWkv6(Args&&... args) { return operate<&ggml_rwkv_wkv6>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr gatedLinearAttn(Args&&... args) { return operate<&ggml_gated_linear_attn>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr rwkvWkv7(Args&&... args) { return operate<&ggml_rwkv_wkv7>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr solveTri(Args&&... args) { return operate<&ggml_solve_tri>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr gatedDeltaNet(Args&&... args) { return operate<&ggml_gated_delta_net>(std::forward<Args>(args)...); }

    template <typename B> [[nodiscard]] UPtr crossEntropyLoss(B&& b) { return operate<&ggml_cross_entropy_loss>(std::forward<B>(b)); }
    template <typename B, typename C> [[nodiscard]] UPtr crossEntropyLossBack(B&& b, C&& c) { return operate<&ggml_cross_entropy_loss_back>(std::forward<B>(b), std::forward<C>(c)); }
    template <typename... Args> [[nodiscard]] UPtr optStepAdamw(Args&&... args) { return operate<&ggml_opt_step_adamw>(std::forward<Args>(args)...); }
    template <typename... Args> [[nodiscard]] UPtr optStepSgd(Args&&... args) { return operate<&ggml_opt_step_sgd>(std::forward<Args>(args)...); }

private:
    // Static Trampoline Bridges
    static void callBouncer(struct ggml_tensor *dst, int ith, int nth, void *userdata) {
        if (auto *p = static_cast<JobGgmlCustomOpPayload*>(userdata)) {
            if (p->func) {
                JobGgmlTensor wrapDst{ dst };
                p->func(wrapDst, ith, nth, p->userdata);
            }
        }
    }

    static void callBouncer1(struct ggml_tensor *dst, const struct ggml_tensor *a, int ith, int nth, void *userdata) {
        if (auto *p = static_cast<JobGgmlCustom1OpPayload*>(userdata)) {
            if (p->func) {
                JobGgmlTensor wrapDst{ dst };
                JobGgmlTensor wrapA{ const_cast<struct ggml_tensor*>(a) };
                p->func(wrapDst, wrapA, ith, nth, p->userdata);
            }
        }
    }

    static void callBouncer2(struct ggml_tensor *dst, const struct ggml_tensor *a, const struct ggml_tensor *b, int ith, int nth, void *userdata) {
        if (auto *p = static_cast<JobGgmlCustom2OpPayload*>(userdata)) {
            if (p->func) {
                JobGgmlTensor wrapDst{ dst };
                JobGgmlTensor wrapA{ const_cast<struct ggml_tensor*>(a) };
                JobGgmlTensor wrapB{ const_cast<struct ggml_tensor*>(b) };
                p->func(wrapDst, wrapA, wrapB, ith, nth, p->userdata);
            }
        }
    }

    static void callBouncer3(struct ggml_tensor *dst, const struct ggml_tensor *a, const struct ggml_tensor *b, const struct ggml_tensor *c, int ith, int nth, void *userdata) {
        if (auto *p = static_cast<JobGgmlCustom3OpPayload*>(userdata)) {
            if (p->func) {
                JobGgmlTensor wrapDst{ dst };
                JobGgmlTensor wrapA{ const_cast<struct ggml_tensor*>(a) };
                JobGgmlTensor wrapB{ const_cast<struct ggml_tensor*>(b) };
                JobGgmlTensor wrapC{ const_cast<struct ggml_tensor*>(c) };
                p->func(wrapDst, wrapA, wrapB, wrapC, ith, nth, p->userdata);
            }
        }
    }

    // Template Executors
    template<auto Function, typename... Args>
    [[nodiscard]] UPtr operate(Args &&...args)
    {
        if (!m_ctx || !m_ctx->isValid() || !isValid()) throw std::runtime_error{ "JobGgmlTensorOp requires a valid tensor and context" };
        (validateArg(args), ...);
        struct ggml_tensor *result = Function(m_ctx->context(), tensor(), nativeArg(std::forward<Args>(args))...);
        if (!result) throw std::runtime_error{ "GGML operation failed to create a tensor" };
        return createUniq(result, m_ctx);
    }

    template<auto Function, typename... Args>
    [[nodiscard]] UPtr rawOperate(Args &&...args)
    {
        if (!m_ctx || !m_ctx->isValid()) throw std::runtime_error{ "JobGgmlTensorOp requires a valid context" };
        (validateArg(args), ...);
        struct ggml_tensor *result = Function(m_ctx->context(), nativeArg(std::forward<Args>(args))...);
        if (!result) throw std::runtime_error{ "GGML operation failed to create a tensor" };
        return createUniq(result, m_ctx);
    }

    template<typename T>
    static void validateArg(const T &value)
    {
        using DecayedT = std::remove_cvref_t<T>;
        if constexpr (std::is_base_of_v<JobGgmlTensor, DecayedT>) {
            if (!value.isValid()) throw std::invalid_argument{ "JobGgmlTensorOp requires valid tensor arguments" };
        } else if constexpr (requires { value->isValid(); }) {
            if (value && !value->isValid()) throw std::invalid_argument{ "JobGgmlTensorOp requires valid tensor arguments" };
        }
    }

    template<typename T>
    [[nodiscard]] static decltype(auto) nativeArg(T &&value) noexcept
    {
        using DecayedT = std::remove_cvref_t<T>;
        if constexpr (std::is_base_of_v<JobGgmlTensor, DecayedT>) {
            return value.tensor();
        } else if constexpr (requires { value->tensor(); }) {
            return value ? value->tensor() : nullptr;
        } else if constexpr (std::is_enum_v<DecayedT>) {
            if constexpr (std::is_same_v<DecayedT, JobGgmlType>)          return static_cast<enum ggml_type>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlPrecision>) return static_cast<enum ggml_prec>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlOpHint>)    return static_cast<enum ggml_op_hint>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlGluOp>)     return static_cast<enum ggml_glu_op>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlUnaryOp>)   return static_cast<enum ggml_unary_op>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlTriType>)   return static_cast<enum ggml_tri_type>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlPoolOp>)    return static_cast<enum ggml_op_pool>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlScaleMode>) return static_cast<enum ggml_scale_mode>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlScaleFlag>) return static_cast<uint32_t>(value);
            else if constexpr (std::is_same_v<DecayedT, JobGgmlSortOrder>) return static_cast<enum ggml_sort_order>(value);
            else return value;
        } else {
            return std::forward<T>(value);
        }
    }

    JobGgmlContext *m_ctx{nullptr}; // Borrowed
};

} // namespace job::ggml