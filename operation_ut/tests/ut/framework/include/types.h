#ifndef _OP_API_UT_COMMON_TYPES_H_
#define _OP_API_UT_COMMON_TYPES_H_

#include "opdev/common_types.h"
#include <string>
#include "acl_base.h"

typedef std::string String;

const std::string& DataTypeToString(aclDataType data_type);
const std::string getFormatName(aclFormat format);
#endif
