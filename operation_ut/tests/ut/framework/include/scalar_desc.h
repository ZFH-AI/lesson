
#ifndef _OP_API_UT_COMMON_SCALAR_DESC_H_
#define _OP_API_UT_COMMON_SCALAR_DESC_H_

#include "nlohmann/json.hpp"
#include "aclnn/aclnn_base.h"
#include "opdev/common_types.h"
//#include "acl_base.h"
#include "printAndLog.h"

using namespace std;
using namespace nlohmann;

class ScalarDesc {
    public:
        // 通常构造函数
        ScalarDesc(bool val = false): f16_(0.0), data_type_(ACL_BOOL), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("") { val_.b = val; }

        ScalarDesc(int8_t val = 0):   f16_(0.0), data_type_(ACL_INT8), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("") { SetInt8Value(&val); }

        ScalarDesc(uint8_t val = 0): f16_(0.0), data_type_(ACL_UINT8), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("") { SetInt8Value(&val); }

        ScalarDesc(int16_t val = 0): f16_(0.0), data_type_(ACL_INT16), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("")  { SetInt16Value(&val); }

        ScalarDesc(uint16_t val = 0): f16_(0.0), data_type_(ACL_UINT16),memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("")  { SetInt16Value(&val); }

        ScalarDesc(int32_t val = 0): f16_(0.0), data_type_(ACL_INT32), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("")  { SetInt32Value(&val); }

        ScalarDesc(uint32_t val = 0): f16_(0.0), data_type_(ACL_UINT32), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("") { SetInt32Value(&val); }

        ScalarDesc(int64_t val = 0): f16_(0.0), data_type_(ACL_INT64),memoryAddress(nullptr),
                                     set_bin_to_value_file_(false), value_("") { SetInt64Value(&val); }

        ScalarDesc(uint64_t val = 0): f16_(0.0), data_type_(ACL_UINT64), memoryAddress(nullptr),
                                      set_bin_to_value_file_(false), value_("") { SetInt64Value(&val); }

        ScalarDesc(float val = 0, aclDataType data_type = ACL_FLOAT, void *memoryAddress = nullptr,
                   bool set_bin_to_value_file = false);

        ScalarDesc(double val = 0): f16_(0.0), data_type_(ACL_DOUBLE), memoryAddress(nullptr),
                                    set_bin_to_value_file_(false), value_("") { val_.d = val; }

        // 拷贝构造函数
        ScalarDesc(const ScalarDesc& desc);
        // 移动构造函数
        ScalarDesc(ScalarDesc&& desc) noexcept;

        ~ScalarDesc() {}

        void ToJson(json& root, bool is_input = true) const;

        // 调用公共接口获取memoryAddress
        void* GetMemoryAddress()  {
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

        void freeMemoryAddress();
        void freeMemoryAddressList() {
            if (memoryAddressList == nullptr) {
                return;
            }
            memoryAddressList = nullptr;
        }

        ScalarDesc& ValueFile(const std::string & binary_file);
        ScalarDesc& InputNodeInfo(const string& node_name, const string& random_type);
        int GetDateTypeSize(aclDataType data_type) const;
    private:
        void SetInt8Value(void * v);
        void SetInt16Value(void * v);
        void SetInt32Value(void * v);
        void SetInt64Value(void * v);

    private:
        union {
            bool b;
            uint64_t i64;
            uint32_t i32;
            uint16_t i16;
            uint8_t i8;
            float f;
            double d;
        } val_;
        float f16_;
        //op::fp16_t f16_;
        aclDataType data_type_;

        void *memoryAddress = nullptr;       // 算子输入的内存地址
        bool set_bin_to_value_file_ = false; // 0：读取框架自动生成的BIN文件，1：读取自定义的BIN文件
        bool gen_data_from_onnx_ = false;    // 是否依赖onnx文件生成的BIN文件
        string value_;                       // BIN文件名称
        string random_type_;                 // 在构造onnx文件输入时的数据类型
        void **memoryAddressList = nullptr;  // 算子输入参数的内存地址：ST使用
};

void DescToJson(json& root, const ScalarDesc& scalar_desc, bool is_input = true);

ScalarDesc *InferAclType(ScalarDesc& scalar_desc);
ScalarDesc *DescToAclContainer(ScalarDesc& scalar_desc);

int ReloadDataFromBinaryFile(ScalarDesc *scalar_desc, size_t index, size_t count, const string& file_prefix,
                             int EXEC_CNT = -1, bool MEM_CACHE_TEST = false);
#endif
