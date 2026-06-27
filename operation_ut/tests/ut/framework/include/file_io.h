#ifndef _OP_API_UT_COMMON_FILE_IO_H_
#define _OP_API_UT_COMMON_FILE_IO_H_

#include <string>
#include <vector>
#include <memory>
#include <iostream>

using namespace std;

void * ReadBinFile(const string& file_name, size_t& size);
int WriteBinFile(const void* host_mem, const string& file_name, size_t size);
bool FileExists(const string& file_name);
void DeleteUtTmpFiles(const string& file_prefix);
void DeleteCwdFilesEndsWith(const string& file_suffix);
string RealPath(const string& path);
void SetUtTmpFileSwitch();
bool GetUtTmpFileSwitch();
bool IsDir(const string& path);
void GetFilesWithSuffix(const string& path, const string& suffix, vector<string>& files);
unique_ptr<char[]> GetBinFromFile(const string& path, uint32_t &data_len);

/**<----   清空编译后的中间文件   ---->**/
struct Enter {
    Enter(const string& info, void(*cleanup_func)(const string&), const string& cleanup_args)
          : info_(info), cleanup_func_(cleanup_func), cleanup_args_(cleanup_args) {
        cout << "=== ENTER " << info_ << " ===" << endl;
    }
    ~Enter() {
        cleanup_func_(cleanup_args_);
        setenv("NEED_AICPU_SIMULATOR", "0", 1);
        cout << "=== LEAVE " << info_ << " ===" << endl;
    }
    private:
        string info_;
        void (*cleanup_func_)(const string&);
        string cleanup_args_;
};
#define ENTER(info) Enter enter(info, DeleteUtTmpFiles, tmp_file_prefix_)

#endif
