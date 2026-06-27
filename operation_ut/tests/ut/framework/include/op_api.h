/**
*  算子调用方式
*   1、void op_api() { op<<<blockDim, l2ctrl, stream>>>(argument list); }
*   2、调用 op_api 函数
**/
#ifndef _OP_API_UT_COMMON_UT_CASE_H_
#define _OP_API_UT_COMMON_UT_CASE_H_

#include <string>
#include <fstream>
#include <map>
#include "gtest/gtest.h"
#include "c_shell.h"
#include "c_py.h"
#include "file_io.h"
#include "tuple_utils.h"
#include "hacl_rt.h"
#include "rts_interface.h"
#include "nlohmann/json.hpp"
#include "printAndLog.h"
#include "config_reader.h"

using namespace std;
using namespace nlohmann;

/** <----------   用例执行宏的类   ----------> **/
template <typename API_F, typename INPUT_T, typename OUTPUT_T, typename ACL_ARGS_T, typename EXTRA_T>
class OpApiUt {
    public:
        OpApiUt(const string& test_suite_name,   // 测试套名称
                const string& test_case_name,    // 测试套中单个用例名称
                const string& op_name,           // 算子名称
                API_F api_func,                  // 算子执行的函数
                const INPUT_T& inputs,           // 算子输入
                const OUTPUT_T& outputs,         // 算子输出
                [[maybe_unused]] const ACL_ARGS_T& wrapper_args,
                const EXTRA_T& extra_args)
                : op_name_(op_name), case_name_(test_suite_name + "_" + test_case_name),
                  api_func_(api_func), inputs_(inputs), outputs_(outputs), extra_args_(extra_args)
        {
                tmp_file_prefix_ = op_name_ + "_" + case_name_;
                case_location_ = GetOpUtSrcPath() + "/" + op_name_;
                input_count_ = tuple_size<INPUT_T>::value;

                Case_Cfg_Json_Root_Dir = GetConfigValueDirByKey("Case_Cfg_Json_Root_Dir");
                Case_InPut_Bin_Root_Dir = GetConfigValueDirByKey("Case_InPut_Bin_Root_Dir");
                Case_OutPut_Bin_Root_Dir = GetConfigValueDirByKey("Case_OutPut_Bin_Root_Dir");

                LogDebug("=== [OpApiUt] TestSuit_CaseName:OP-NAME " + case_name_ + ":" + op_name_ + " ===");
                LogDebug("[OpApiUt] Op Number of input parameters :" + to_string(input_count_));
        }

        ~OpApiUt() {}

        OpApiUt& InputGenFunc(const string& input_gen_func) {
            input_gen_func_ = input_gen_func;
            return *this;
        }

        // 算子文件执行次数
        void SetExceNum(uint32_t exce_num) {
             ASSERT_GT(exce_num, 0);
             EXEC_NUM = exce_num;
        }
        // 算子执行在哪个机器核上
        void SetBLOCK_DIM(uint32_t block_dim) {
            ASSERT_GT(block_dim, 0);
            BLOCK_DIM_NUM = block_dim;
        }
        // 算子依赖的onnx文件
        void SetOnnxFile(const string& onnx_file_path) {
            onnx_file_path_ = onnx_file_path;
        }

        void TestPrecision() {
            //ENTER("TestPrecision");
            ConvenientInplaceMap();
            ToJsonFile();
            ASSERT_EQ(GenerateInput(), 0);
            ASSERT_EQ(GenerateGolden(), 0);
            ASSERT_EQ(GenDataFromOnnx(), 0);
            GetConvertedAclArgs();
            ASSERT_EQ(ReloadTensorDataToDevice(), 0);

            LogDebug("[OpApiUt] rtStreamCreate ...");
            rtError_t ret = rtStreamCreate(&stream_, 0);
            if (ret != RT_ERROR_NONE) {
                LogError("[OpApiUt] rtStreamCreate failed, errocr code is " + to_string(ret));
            }
            ASSERT_EQ(ret, RT_ERROR_NONE);

            LogDebug("[OpApiUt] run the cce operator code..");
            for (int i = 0; i < EXEC_NUM; i++){
                CallApiInternal(api_func_);

                LogDebug("index = [" + to_string(i + 1) + "] rtStreamSynchronize ...");
                ASSERT_EQ(rtStreamSynchronize(stream_), RT_ERROR_NONE);

                LogDebug("index = [" + to_string(i + 1) + "] get output from npu ...");
                ASSERT_EQ(SaveResultFromDevice(-1), 0);

                LogDebug("index = [" + to_string(i + 1) + "] compare output with golden ...");
                ASSERT_EQ(CompareGolden(), 0);
            }

            /*<--  释放资源 -->*/
            LogDebug("[OpApiUt] resource free stream ...");
            rtStreamDestroy(stream_);

            LogDebug("[OpApiUt] resource free device memory address ...");
            SafeFreeAllMemoryAddresses();

            LogDebug("[OpApiUt] resource free all_args_ ...");
            FreeTuplePointers(all_args_);
        }

    private:
        void ConvenientInplaceMap() {
            if (tuple_size<OUTPUT_T>::value <= 0) {
                inplace_map_ = {{"0", 0}};
                inplace_map_reverse_ = {{0, 0}};
                inplace_output_.insert(0);
            }
        }

        void ToJsonFile() const {
            json root;
            root["op"] = op_name_;
            root["case_name"] = case_name_;
            root["case_location"] = case_location_;
            if (!input_gen_func_.empty()) {
                root["input_gen_func"] = input_gen_func_;
            }

            if (inplace_map_.size() > 0) {
                root["inplace"] = inplace_map_;
            }

            json input_desc = json::array();
            ConvertDescToJson(input_desc, inputs_, true);
            root["input_desc"] = input_desc;

            json output_desc = json::array();
            ConvertDescToJson(output_desc, outputs_, false);
            root["output_desc"] = output_desc;

            if (tuple_size<EXTRA_T>::value > 0) {
                json extra_desc = json::array();
                ConvertDescToJson(input_desc, inputs_, false);
                root["extra_args"] = output_desc;
            }
            string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
            ofstream outFile(json_file);
            if (!outFile) {
               throw std::runtime_error("ToJsonFile Failed");
            } else {
                outFile << setw(2) << root;
                outFile.close();
            }
        }

        // Tensor的JSON文件
        int GenerateInput() {
            string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
            return PyScripts::GetInstance().GenerateInput(json_file);
        }


        // 判断 case的 golden 脚本是否存在
        int GenerateGolden() {
            golden_exists_ = true;
            // 取得当前op_golden_path的父目录
            string op_golden_path = GetOpUtSrcPath();
            size_t pos = op_golden_path.find_last_of('/');
            if (pos == std::string::npos) {
                LogError("[OpApiUt][GenerateGolden] Please Check Whether OP_API_UT_SRC_DIR Is Set" + op_golden_path);
                return 0;
            }
            string op_golden_py_file = op_golden_path.substr(0, pos) + "/golden/" + op_name_ + ".py";
            if (FileExists(op_golden_py_file)) {
                golden_exists_ = true;
                string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
                return PyScripts::GetInstance().GenerateGolden(json_file, op_golden_py_file, "gen_golden");
            } else {
                golden_exists_ = true;
                std::string strInfo = "[OpApiUt]If the operator output is customized, ignore this information. Otherwise"
                                      " Please Check Location File Is Exist [" + op_golden_py_file + "]";
                LogDebug(strInfo);
                LogInfo(strInfo);
                return 0;
            }
        }

        int GenDataFromOnnx() {
            if (onnx_file_path_.empty()) {
                return 0;
            }
            string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
            return PyScripts::GetInstance().GenDataFromOnnx(onnx_file_path_, json_file);
        }

        ACL_ARGS_T GetConvertedAclArgs() {
            if (!all_args_set_) {
                all_args_ = ConvertDescToAclTypes(tuple_cat(inputs_, outputs_));
                all_args_set_ = true;
            }
            return all_args_;
        }

        int ReloadTensorDataToDevice() {
            string input_bin_file = Case_InPut_Bin_Root_Dir + tmp_file_prefix_;
            return ReloadTensorData(all_args_, input_count_, input_bin_file, -1, false);
        }

        // 从TensorDesc中提取memoryAddress的辅助函数
        template <typename T>
        static auto GetMemoryAddress(const T &desc) {
            // printf("op_api desc->GetMemoryAddress() %p\n", desc->GetMemoryAddress());
            return desc->GetMemoryAddress();
        }

        // 转换TensorDesc到memoryAddress
        template <typename Tuple, std::size_t... I>
        auto ConvertDescToMemoryAddress(Tuple&& t, std::index_sequence<I...>) {
            return std::make_tuple(GetMemoryAddress(std::get<I>(t))...);
        }

        // 获取转换后的参数元组（memoryAddress）
        auto GetConvertedOpFuncArgs() {
            return ConvertDescToMemoryAddress(
                all_args_,
                std::make_index_sequence<std::tuple_size_v<decltype(all_args_)>>{}
            );
        }

        template <typename ApiFunc>
        void CallApiInternal(ApiFunc&& api_func) {
            // 获取转换后的memoryAddress元组
            auto memory_args = GetConvertedOpFuncArgs();  // (input1Addr, input2Addr, outputAddr)
            // 合并固定参数和动态参数
            auto args = std::tuple_cat(
                std::make_tuple(BLOCK_DIM_NUM, stream_),  // 固定参数 (blockDim, stream)
                memory_args                               // 动态参数，类似(input1Addr, input2Addr, outputAddr)
            );
            // 调用op_api
            std::apply(std::forward<ApiFunc>(api_func), args);
        }

        int SaveResultFromDevice(int index) {
            string output_bin_file = Case_OutPut_Bin_Root_Dir + tmp_file_prefix_;
            return SaveResult(all_args_, input_count_, output_bin_file, inplace_map_reverse_, inplace_output_, index);
        }

        int CompareGolden() {
            if (!golden_exists_) {
                return 0;
            }
            string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
            return PyScripts::GetInstance().CompareGolden(json_file);
        }
        // 释放内存资源
        template <typename T>
        static void SafeFreeMemoryAddress(T &desc) {
            if (desc->GetMemoryAddress() != nullptr) {
                desc->freeMemoryAddress();
            }
        }

        template <typename Tuple, std::size_t... I>
        void SafeFreeAllMemoryAddressesImpl(Tuple&& t, std::index_sequence<I...>) {
            (SafeFreeMemoryAddress(std::get<I>(t)), ...);
        }

        void SafeFreeAllMemoryAddresses() {
            constexpr size_t argNum = std::tuple_size_v<decltype(all_args_)>;
            SafeFreeAllMemoryAddressesImpl(
                all_args_,
                std::make_index_sequence<argNum>{}
            );
        }
        // 释放元组中的指针
        template<typename Tuple, size_t... I>
        void FreeTuplePointersImpl(Tuple& t, std::index_sequence<I...>) {
            (..., (delete std::get<I>(t)));  // C++17 折叠表达式
        }

        template<typename Tuple>
        void FreeTuplePointers(Tuple& t) {
            FreeTuplePointersImpl(t, std::make_index_sequence<std::tuple_size<Tuple>::value>{});
        }
    private:
        bool golden_exists_ = true;
        string op_name_;
        string case_name_;
        string input_gen_func_;
        string tmp_file_prefix_;
        string case_location_;
        string onnx_file_path_;
        // actually it is difficult to describe inplace map in condition: 3 outputs & 1 inplace input (how to define the
        // output index? 1 possible soultion is that: consider output index as which in the golden output file index.)
        // Note: If no OUTPUT is specified, default inplace map will be output0 -> input0 only.
        map<string, short int> inplace_map_; // output_index -> input_index
        map<short int, short int> inplace_map_reverse_; // input_index -> output_index
        set<short int> inplace_output_; // inplace output_index

        string Case_Cfg_Json_Root_Dir;    // Case配置的input/output参数信息保存目录
        string Case_InPut_Bin_Root_Dir;   // 算子输入BIN文件的保存目录
        string Case_OutPut_Bin_Root_Dir;  // 算子执行完毕之后输出结果的保存目录

        API_F api_func_;
        INPUT_T inputs_;
        OUTPUT_T outputs_;
        ACL_ARGS_T all_args_;  // only inputs + outputs_.
        EXTRA_T extra_args_;
        bool all_args_set_ = false;
        size_t input_count_;
        rtStream_t stream_ = nullptr;

        uint32_t EXEC_NUM = 1;
        uint32_t BLOCK_DIM_NUM = 1;
};

#define OP_API_UT(op_name, input, output, ...)  \
    OpApiUt(testing::UnitTest::GetInstance()->current_test_info()->test_case_name(),  \
            testing::UnitTest::GetInstance()->current_test_info()->name(), \
            #op_name,  \
            op_name##_api,  \
            input,  \
            output,  \
            InferAclTypes(tuple_cat(input, output)),  \
            make_tuple(__VA_ARGS__))

#define INPUT(...)  make_tuple(__VA_ARGS__)
#define OUTPUT(...) make_tuple(__VA_ARGS__)

#endif
