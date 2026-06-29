/**
 算子调用方式
 rtDevBinaryRegister -> rtFunctionRegister -> rtKernelLaunch
**/
#ifndef _OP_KERNEL_UT_COMMON_UT_CASE_H_
#define _OP_KERNEL_UT_COMMON_UT_CASE_H_

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
#include <acl/acl.h>
#include "printAndLog.h"
#include "config_reader.h"

using namespace std;
using namespace nlohmann;

/*<-- 用例执行类  -->*/
template<typename INPUT_T, typename OUTPUT_T, typename ACL_ARGS_T, typename EXTRA_T>
class OpKernelUt {
    public:
        OpKernelUt(const string& test_suite_name,   // 测试套名称
                   const string& test_case_name,    // 测试套中单个用例名称
                   const string& op_name,           // 算子.o的文件名称
                   const string& op_name_func,      // 算子.o中的函数名称
                   const INPUT_T& inputs,           // 算子输入
                   const OUTPUT_T& outputs,         // 算子输出
                   [[maybe_unused]] const ACL_ARGS_T& wrapper_args,
                   const EXTRA_T& extra_args)
                : op_name_(op_name), op_name_func_(op_name_func), case_name_(test_suite_name + "_" + test_case_name),
                  inputs_(inputs), outputs_(outputs), extra_args_(extra_args)
        {
                tmp_file_prefix_ = op_name_ + "_" + case_name_;
                case_location_ = GetOpUtSrcPath() + "/" + op_name_;
                input_count_ = tuple_size<INPUT_T>::value;
               
                Case_Cfg_Json_Root_Dir = GetConfigValueDirByKey("Case_Cfg_Json_Root_Dir");
                Case_InPut_Bin_Root_Dir = GetConfigValueDirByKey("Case_InPut_Bin_Root_Dir");
                Case_OutPut_Bin_Root_Dir = GetConfigValueDirByKey("Case_OutPut_Bin_Root_Dir");

                LogDebug("=== [OpKernelUt] TestSuit_CaseName:OP-NAME " + case_name_ + ":" + op_name_ + " ===");
                LogDebug("[OpKernelUt] Op Number of input parameters :" + to_string(input_count_));
        }
        ~OpKernelUt() {}
        // 算子文件执行次数
        void SetExceNum(uint32_t exce_num) {
            ASSERT_GT(exce_num, 0);
            EXEC_CNT = exce_num;
        }
        // 算子执行在哪个机器核上
        void SetBLOCK_DIM(uint32_t block_dim) {
            ASSERT_GT(block_dim, 0);
            BLOCK_DIM = block_dim;
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

            LogDebug("[OpKernelUt] get operator.o file path ...");
            ASSERT_EQ(GetOpDynamicFilePath(),0);

            LogDebug("[OpKernelUt] rtStreamCreate ...");
            rtStream_t stream;
            rtError_t ret = rtStreamCreate(&stream, 0);
            ASSERT_EQ(ret, RT_ERROR_NONE);

            // 算子调用代码
            // 1、读取算子.o文件
            LogDebug("[OpKernelUt] reading operator.o binary ...");
            size_t size = 0;
            void* host_mem = ReadBinFile(op_o_path_.c_str(), size);
            ASSERT_NE(host_mem, nullptr);

            // 2、注册算子信息
            LogDebug("[OpKernelUt] registering binary ...");
            rtDevBinary_t dev_bin;
            const char* funcName = op_name_func_.c_str();
            void * dev_bin_handle = NULL;
            ret = RT_ERROR_NONE;
            {
                dev_bin.data = host_mem;
                dev_bin.length = size;
                dev_bin.magic = RT_DEV_BINARY_MAGIC_ELF;
                dev_bin.version = 0;
                ret = rtDevBinaryRegister(&dev_bin, &dev_bin_handle);
                ASSERT_EQ(ret, RT_ERROR_NONE);
                ret = rtFunctionRegister(dev_bin_handle, funcName, funcName, (void *) funcName, 0);
                ASSERT_EQ(ret, RT_ERROR_NONE);
            }
            // 3、调用算子信息
            LogDebug("[OpKernelUt] rtKernelLaunch ...");
            uint32_t argNum = std::tuple_size_v<decltype(all_args_)>;
            void* funcArgs[argNum];
            GetConvertedOpFuncArgs(funcArgs);
            //for (int i = 0; i < argNum; i++) {
            //    printf("funcArgs[%d] = %p\n", i, funcArgs[i]);
            //}
            for (int i = 0; i < EXEC_CNT; i++){
                rtError_t ret = rtKernelLaunch((void *)funcName, BLOCK_DIM, (void *)(&funcArgs), sizeof(funcArgs), nullptr, stream);
                ASSERT_EQ(ret, RT_ERROR_NONE);

                LogDebug("index = [" + to_string(i + 1) + "] rtStreamSynchronize ...");
                ASSERT_EQ(rtStreamSynchronize(stream), RT_ERROR_NONE);

                LogDebug("index = [" + to_string(i + 1) + "] get output from npu ...");
                ASSERT_EQ(SaveResultFromDevice(-1), 0);

                LogDebug("index = [" + to_string(i + 1) + "] compare output with golden ...");
                ASSERT_EQ(CompareGolden(), 0);
            }

            /*<--  释放资源 -->*/
            LogDebug("[OpKernelUt] resource free stream ...");
            rtStreamDestroy(stream);

            LogDebug("[OpKernelUt] resource free device memory address ...");
            SafeFreeAllMemoryAddresses();

            LogDebug("[OpKernelUt] resource free all_args_ ...");
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
            root["op_in_function"] = op_name_func_;
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

        ACL_ARGS_T GetConvertedAclArgs() {
            if (!all_args_set_) {
                all_args_ = ConvertDescToAclTypes(tuple_cat(inputs_, outputs_));
                all_args_set_ = true;
            }
            return all_args_;
        }

        // 从TensorDesc中提取memoryAddress的辅助函数
        template <typename T>
        static void* GetMemoryAddress(const T &desc) {
            return desc->GetMemoryAddress();
        }

        // 转换TensorDesc到memoryAddress并存储到数组中
        template <typename Tuple, std::size_t... I>
        void ConvertDescToMemoryAddress(Tuple&& t, std::index_sequence<I...>, void** funcArgs) {
            // 使用折叠表达式将每个元素的结果存入数组
            ((funcArgs[I] = GetMemoryAddress(std::get<I>(t))), ...);
        }
        
        // 获取转换后的参数数组（memoryAddress）
        void GetConvertedOpFuncArgs(void** funcArgs) {
            constexpr size_t argNum = std::tuple_size_v<decltype(all_args_)>;
            ConvertDescToMemoryAddress(
                all_args_,
                std::make_index_sequence<argNum>{},
                funcArgs
            );
        }

        // 通过配置的JSON文件，随机生成Tensor数据
        int GenerateInput() {
            string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
            return PyScripts::GetInstance().GenerateInput(json_file);
        }
        
        // 判断 case的 golden 脚本是否存在
        int GenerateGolden() {
            golden_exists_ = true;
            // 取得当前op_golden_path的父目录
            const std::string op_golden_path = GetOpUtSrcPath();
            const size_t pos = op_golden_path.find_last_of('/');
            if (pos == std::string::npos) {
                LogError("[OpKernelUt][GenerateGolden] Please Check Whether OP_API_UT_SRC_DIR Is Set" + op_golden_path);
                return 0;
            }
            const std::string op_golden_py_file = op_golden_path.substr(0, pos) + "/golden/" +
                                                  op_name_+ "_"+ op_name_func_ + ".py";
            if (FileExists(op_golden_py_file)) {
                golden_exists_ = true;
                string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
                return PyScripts::GetInstance().GenerateGolden(json_file, op_golden_py_file, "gen_golden");
            } else {
                golden_exists_ = true;
                std::string strInfo = "[OpKernelUt]If the operator output is customized, ignore this information. Otherwise"
                                      "Please Check Location File Is Exist [" + op_golden_py_file + "]";
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
        // 算子输入：Host -> Device
        int ReloadTensorDataToDevice() {
            string input_bin_file = Case_InPut_Bin_Root_Dir + tmp_file_prefix_;
            return ReloadTensorData(all_args_, input_count_, input_bin_file, -1, false);
        }
        
        // 获取该算子编译的.o文件的路径
        int GetOpDynamicFilePath() {
            // 获取算子.o文件的路径项目移植时按需修改为当前.o的真实路径
            const std::string fullPath = GetOpUtSrcPath();
            const size_t pos = fullPath.find("/OP/");    // 如果项目名称修改这里也需要修改
            int ret =1;
            if (pos == std::string::npos) {
                LogError("[OpKernelUt][GetOpDynamicFilePath]Failed Please Check Whether OP_API_UT_SRC_DIR Is Set " + fullPath);
                return ret;
            }
            const std::string op_o_path = fullPath.substr(0, pos + 3) + "/build/tests/" + op_name_ + ".o";
            // 判断提供的 op.o是否存在
            if (FileExists(op_o_path)) {
                op_o_path_ = op_o_path;
                ret = 0;
                LogDebug("[OpKernelUt] get op binary file path success.  " + op_o_path);

            } else {
                ret = 1;
                LogDebug("[OpKernelUt] get op binary file path failed. " + op_o_path);
                LogError("[OpKernelUt] get op binary file path failed. " + op_o_path);
            }
            return ret;
        }
        // 算子输出：DEVICE -> Host测试
        int SaveResultFromDevice(int index) {
            string output_bin_file = Case_OutPut_Bin_Root_Dir + tmp_file_prefix_;
            //return SaveResult(all_args_, input_count_, tmp_file_prefix_, inplace_map_reverse_, inplace_output_, index);
            return SaveResult(all_args_, input_count_, output_bin_file, inplace_map_reverse_, inplace_output_, index);
        }

        int CompareGolden() {
            if (!golden_exists_) {
                return 0;
            }
            const std::string json_file = Case_Cfg_Json_Root_Dir + tmp_file_prefix_ + ".json";
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
        string op_name_;  // 编译出来的算子.o的文件名称，例如 conv.o， op_name_ = conv
        string op_name_func_;  // 算子.o中的函数名称，例如 conv.o中的 func_a函数
        string case_name_;
        string input_gen_func_;
        string tmp_file_prefix_;
        string case_location_;
        string op_o_path_;
        string onnx_file_path_;

        map<string, short int> inplace_map_; // output_index -> input_index
        map<short int, short int> inplace_map_reverse_; // input_index -> output_index
        set<short int> inplace_output_; // inplace output_index

        INPUT_T inputs_;
        OUTPUT_T outputs_;
        ACL_ARGS_T all_args_;  // only inputs + outputs_.
        EXTRA_T extra_args_;
        bool all_args_set_ = false;
        size_t input_count_;

        string Case_Cfg_Json_Root_Dir;    // Case配置的input/output参数信息保存目录
        string Case_InPut_Bin_Root_Dir;   // 算子输入BIN文件的保存目录
        string Case_OutPut_Bin_Root_Dir;  // 算子执行完毕之后输出结果的保存目录

        uint32_t EXEC_CNT = 1;
        uint32_t BLOCK_DIM = 1;
};

/*<-- 测试使能宏定义 -->*/
#define OP_KERNEL_UT(op_o_name, op_o_in_func, input, output, ...)  \
    OpKernelUt(testing::UnitTest::GetInstance()->current_test_info()->test_case_name(),  \
               testing::UnitTest::GetInstance()->current_test_info()->name(),  \
               #op_o_name,  \
               #op_o_in_func,  \
               input,  \
               output,  \
               InferAclTypes(tuple_cat(input, output)),  \
               make_tuple(__VA_ARGS__))

#define INPUT(...)  make_tuple(__VA_ARGS__)
#define OUTPUT(...) make_tuple(__VA_ARGS__)
#endif
