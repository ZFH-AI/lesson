#ifndef _OP_API_UT_COMMON_TUPLE_UTILS_H_
#define _OP_API_UT_COMMON_TUPLE_UTILS_H_

#include <tuple>
#include <assert.h>
#include "nlohmann/json.hpp"
#include "opdev/common_types.h"
#include "tensor_desc.h"
#include "scalar_desc.h"
#include "rts_interface.h"

using namespace std;
using namespace nlohmann;

template<typename T>
inline void DescToJson(json& root, const T& t, [[maybe_unused]] bool is_input = true) {
    json x;
    stringstream ss;
    if constexpr (is_pointer_v<T>) {
        if (t == nullptr) {
            ss << "None";
        } else if constexpr (std::is_same_v<std::decay_t<T>, char *> || std::is_same_v<std::decay_t<T>, const char *>) {
            ss << t;
        } else {
            assert(false && "Not support yet !!");
        }
    } else if (typeid(T) == typeid(uint8_t)) {
        ss << static_cast<uint>(t);
    } else if (typeid(T) == typeid(int8_t)) {
        ss << static_cast<int>(t);
    } else {
        ss << t;
    }
    x["value"] = ss.str();
    x["dtype"] = typeid(t).name();
    root.push_back(x);
}
template<typename... T>
inline void ConvertDescToJson(json& root, const tuple<T...>& t, bool is_input = true) {
    apply([&root, &is_input](const auto&... args) {(DescToJson(root, args, is_input), ...);}, t);
}

/*<-- InferAclTypes 部分  -->*/
template<typename T>
inline T InferAclType(const T& t) {
    return t;
}

template <typename T>
inline constexpr auto InferAclTypes(T t) {
    constexpr auto size = tuple_size<T>::value;
    return InferAclTypes(t, make_index_sequence<size>{});
}

template<typename T, size_t ... I>
inline constexpr auto InferAclTypes(T t, index_sequence<I ...>) {
    return make_tuple(InferAclType(get<I>(t)) ...);
}

/*<-- ConvertDescToAclTypes 部分  -->*/
template<typename T>
inline T DescToAclContainer(const T& t) {
    return t;
}

template <typename T>
constexpr auto ConvertDescToAclTypes(T t) {
    constexpr auto size = tuple_size<T>::value;
    return ConvertDescToAclTypes(t, make_index_sequence<size>{});
}

template<typename T, size_t ... I>
constexpr auto ConvertDescToAclTypes(T t, index_sequence<I ...>) {
    return make_tuple(DescToAclContainer(get<I>(t)) ...);
}

/**<----  reload tensor data  ---->**/
template<typename T>
int ReloadDataFromBinaryFile([[maybe_unused]] T arg, [[maybe_unused]] size_t index,
    [[maybe_unused]] size_t total_input, [[maybe_unused]] const string& file_prefix,
    [[maybe_unused]] int EXEC_CNT, [[maybe_unused]] bool MEM_CACHE_TEST) {
    return 0;
}

template<size_t index, typename... Ts>
struct ReloadIterator {
    int operator() (tuple<Ts...>& t, size_t input_count, const string& file_prefix, int EXEC_CNT, bool MEM_CACHE_TEST) {
        auto ret = ReloadDataFromBinaryFile(get<index>(t), index, input_count, file_prefix, EXEC_CNT, MEM_CACHE_TEST);
        if (ret != 0) {
            return ret;
        }
        return ReloadIterator<index - 1, Ts...>{}(t, input_count, file_prefix, EXEC_CNT, MEM_CACHE_TEST);
    }
};

template<typename... Ts>
struct ReloadIterator<0, Ts...> {
    int operator() (tuple<Ts...>& t, size_t input_count, const string& file_prefix, int EXEC_CNT, bool MEM_CACHE_TEST) {
        return ReloadDataFromBinaryFile(get<0>(t), 0, input_count, file_prefix, EXEC_CNT, MEM_CACHE_TEST);
    }
};

template<typename... Ts>
int ReloadTensorData(tuple<Ts...>& t, size_t input_count, const string& file_prefix, int EXEC_CNT, bool MEM_CACHE_TEST) {
    const auto size = tuple_size<tuple<Ts...>>::value;
    ReloadIterator<size - 1, Ts...> it;
    return it(t, input_count, file_prefix, EXEC_CNT, MEM_CACHE_TEST);
}


/**<----  save result         ---->**/
[[maybe_unused]]static short int GetInplaceOutputIdx(size_t index, size_t input_count,
                                                     const map<short int, short int>& inplace_map_reserve) {
    if (index >= input_count) {
        return -1;
    }
    auto it = inplace_map_reserve.find(static_cast<short int>(index));
    return it != inplace_map_reserve.end() ? it->second : -1;
}

template<typename T>
int SaveResultToBinaryFile([[maybe_unused]] T arg, [[maybe_unused]] size_t index,
                           [[maybe_unused]] size_t total_input,
                           [[maybe_unused]] const string& file_prefix,
                           [[maybe_unused]] const set<short int>& inplace_output,
                           [[maybe_unused]] int array_subscript) {
    return 0;
}

template<typename T>
int SaveResultToBinaryFile([[maybe_unused]] T arg, [[maybe_unused]] size_t index,
                           [[maybe_unused]] const string& file_prefix,
                           [[maybe_unused]] int array_subscript) {
    return 0;
}

template<size_t index, typename... Ts>
struct SaveIterator {
    int operator() (tuple<Ts...>& t, size_t input_count, const string& file_prefix, const map<short int,
                    short int>& inplace_map_reserve, const set<short int>& inplace_output, int array_subscript) {
        short int inplace_output_idx = GetInplaceOutputIdx(index, input_count, inplace_map_reserve);
        int ret = 0;
        if (inplace_output_idx < 0) {
            ret = SaveResultToBinaryFile(get<index>(t), index, input_count, file_prefix, inplace_output, array_subscript);
        } else {
            ret = SaveResultToBinaryFile(get<index>(t), inplace_output_idx, file_prefix, array_subscript);
        }

        if (ret != 0) {
            cout << "Save Device Output To Host Failed .." << endl;
            return ret;
        }
        return SaveIterator<index - 1, Ts...>{}(t, input_count, file_prefix, inplace_map_reserve, inplace_output, array_subscript);
    }
};

template<typename... Ts>
struct SaveIterator<0, Ts...> {
    int operator() (tuple<Ts...>& t, size_t input_count, const string& file_prefix, const map<short int,
                    short int>& inplace_map_reserve, const set<short int>& inplace_output, int array_subscript) {
        short int inplace_output_idx = GetInplaceOutputIdx(0, input_count, inplace_map_reserve);
        if (inplace_output_idx < 0) {
            return SaveResultToBinaryFile(get<0>(t), 0, input_count, file_prefix, inplace_output, array_subscript);
        } else {
            return SaveResultToBinaryFile(get<0>(t), inplace_output_idx, file_prefix, array_subscript);
        }
    }
};

template<typename... Ts>
int SaveResult(tuple<Ts...>& t, size_t input_count, const string& file_prefix, const map<short int,
               short int>& inplace_map_reserve, const set<short int>& inplace_output, int array_subscript) {
    const auto size = tuple_size<tuple<Ts...>>::value;
    SaveIterator<size - 1, Ts...> it;
    return it(t, input_count, file_prefix, inplace_map_reserve, inplace_output, array_subscript);
}


#endif
