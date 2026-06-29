#ifndef _OP_API_UT_COMMON_TENSOR_DESC_H_
#define _OP_API_UT_COMMON_TENSOR_DESC_H_

#include <sstream>
#include <set>
#include "nlohmann/json.hpp"
//#include "acl_base.h"
#include "opdev/common_types.h"
#include "printAndLog.h"

using namespace std;
using namespace nlohmann;

class TensorDesc {
    public:
        // 通常构造函数
        TensorDesc(const vector<int64_t>& view_dims = {1, 1, 16, 16},
                    aclDataType data_type = ACL_FLOAT,
                    aclFormat format = ACL_FORMAT_NCHW,
                    const vector<int64_t>& stride_ = {},
                    int64_t offset = 0,
                    const vector<int64_t>& storage_dims = {},
                    void *memoryAddress = nullptr,
                    void **memoryAddressList = nullptr,
                    bool set_bin_to_value_file = false);

        // 拷贝构造函数
        TensorDesc(const TensorDesc& desc);
        TensorDesc& operator=(const TensorDesc& desc) {
            if (this != &desc) {
                view_dims_ = desc.view_dims_;
                data_type_ = desc.data_type_;
                format_ = desc.format_;
                stride_ = desc.stride_;
                offset_ = desc.offset_;
                storage_dims_ = desc.storage_dims_;
                precision_ = desc.precision_;
                value_range_ = desc.value_range_;
                value_ = desc.value_;
                valid_count_ = desc.valid_count_;
                tensorSize = desc.tensorSize;
                memoryAddress = desc.memoryAddress;
                memoryAddressList = desc.memoryAddressList;
                set_bin_to_value_file_ = desc.set_bin_to_value_file_;
                gen_data_from_onnx_ = desc.gen_data_from_onnx_;
                precision_typeid = desc.precision_typeid;
            }
            return *this;
        }

//        // 确保移动语义正确
//        TensorDesc(const TensorDesc && desc) noexcept;
//        TensorDesc& operator=(TensorDesc&& desc) noexcept {
//             if (this != &desc) {
//                view_dims_ = std::move(desc.view_dims_);
//                data_type_ = desc.data_type_;
//                format_ = desc.format_;
//                stride_ = std::move(desc.stride_);
//                offset_ = desc.offset_;
//                storage_dims_ = std::move(desc.storage_dims_);
//                precision_ = desc.precision_;
//                value_range_ = desc.value_range_;
//                value_ = desc.value_;
//                valid_count_ = desc.valid_count_;
//                tensorSize = desc.tensorSize;
//                memoryAddress = desc.memoryAddress;
//                set_bin_to_value_file_ = desc.set_bin_to_value_file_;
//              gen_data_from_onnx_ = desc.gen_data_from_onnx_;
//              precision_typeid = desc.precision_typeid;
//              memoryAddressList = desc.memoryAddressList;
//
//                desc.memoryAddress = nullptr;
//                desc.view_dims_.clear();
//                desc.stride_.clear();
//                desc.storage_dims_.clear();
//            }
//            return *this;
//        }

        ~TensorDesc();

        void ToJson(json& root, bool is_input = true) const;

        TensorDesc& ViewDims(const vector<int64_t>& view_dims);

        TensorDesc& Format(aclFormat format);

        // 自定义精度对比
        TensorDesc& Precision(const std::vector<float>& precision_value = {1e-03, 0.2, 1e-08}, int precision_mode = 0);

        template<typename T>
        TensorDesc& ValueRange(T low, T high) {
            assert(low <= high);
            stringstream ss;
            if (typeid(T) == typeid(uint8_t)) {
                ss << "[" << (uint)low << "," << (uint)high << "]";
            } else if (typeid(T) == typeid(int8_t)) {
                ss << "[" << (int)low << "," << (int)high << "]";
            } else {
                ss << "[" << low << "," << high << "]";
            }
            value_range_.clear();
            ss >> value_range_;
            return *this;
        }

        template<typename T>
        TensorDesc& Value(const vector<T>& v) {
            stringstream ss;
            ss << "[";
            for (size_t i = 0; i < v.size(); ++i) {
                if (typeid(T) == typeid(uint8_t)) {
                    ss << (uint)v[i];
                } else if (typeid(T) == typeid(int8_t)) {
                    ss << (int)v[i];
                } else {
                    ss << v[i];
                }

                if (i < v.size() - 1) ss << ",";
            }
            ss << "]";
            value_.clear();
            ss >> value_;
            return *this;
        }

        TensorDesc& ValueFile(const std::string & binary_file);

        TensorDesc& ValidCount(int32_t cnt);

        TensorDesc& InputNodeInfo(const string& node_name, const string& random_type);

        // 调用公共接口获取memoryAddress
        void* GetMemoryAddress() {
            return memoryAddress;
        }
        // 调用公共接口获取memoryAddressList[i]
        void* GetMemoryAddressListByIndex(uint32_t i) {
            return memoryAddressList[i];
        }
        // 调用公共接口获取memoryAddressList
        void** GetMemoryAddressList() {
            return memoryAddressList;
        }
        // 释放执行DEVICE侧的指针
        void freeMemoryAddress();
        void freeMemoryAddressList() {
            if (memoryAddressList == nullptr) {
                return;
            }
            memoryAddressList = nullptr;
        }

        int64_t GetViewCount(vector<int64_t> &view_dims) const;
        int64_t GetStorageCount() const;
        int32_t GetDateTypeSize(aclDataType data_type) const;
    private:
        vector<int64_t> view_dims_;
        aclDataType data_type_;
        aclFormat format_;
        vector<int64_t> stride_;
        int64_t offset_;
        vector<int64_t> storage_dims_;

        string precision_;
        string value_range_ = "[-2,2]";
        string value_;
        string random_type_;                // 在构造onnx文件输入时的数据类型
        int32_t valid_count_ = -1;
        int32_t precision_typeid = 0;       // 自定义精度对比策略选择ID
        
        uint64_t tensorSize=0;              // view_dims_中的元素乘积
        void *memoryAddress = nullptr;      // 算子输入参数的内存地址：UT使用
        void **memoryAddressList = nullptr; // 算子输入参数的内存地址：ST使用
        bool set_bin_to_value_file_=false;  // 0：读取框架自动生成的BIN文件，1：读取自定义的BIN文件
        bool gen_data_from_onnx_ = false;   // 是否依赖onnx文件生成的BIN文件
};

TensorDesc *InferAclType(TensorDesc& tensor_desc);
TensorDesc *DescToAclContainer(TensorDesc& tensor_desc);


void DescToJson(json& root, const TensorDesc& tensor_desc, bool is_input = true);

int ReloadDataFromBinaryFile(TensorDesc* tensor_desc, size_t index, size_t count, const string& file_prefix,
                             int EXEC_CNT = -1, bool MEM_CACHE_TEST = false);

int SaveResultToBinaryFile(TensorDesc* tensor_desc, size_t index, const string& file_prefix, int array_subscript = -1);

int SaveResultToBinaryFile(TensorDesc* tensor_desc, size_t index, size_t total_input, const string& file_prefix,
                           const set<short int>& inplace_output,  int array_subscript = -1);

#endif
