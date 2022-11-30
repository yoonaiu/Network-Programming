#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <stdint.h>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <vector>

using namespace std;


uint32_t SwapEndian_32(uint32_t val)
{
    val = (val<<24) | ((val<<8) & 0x00ff0000) |
          ((val>>8) & 0x0000ff00) | (val>>24);
    return val;
}

/*
50414B4F = ori
4F000000 | = ori << 24
004B0000 | = 414B4F00 & 00ff0000 = ori << 8 & 00ff0000
00004100 | = 0050414B & 0000ff00 = ori >> 8 & 0000ff00
00000050 | = ori >> 24
*/

uint64_t SwapEndian_64(uint64_t val)
{
    val = ((val & 0x00000000FFFFFFFFull) << 32) | ((val & 0xFFFFFFFF00000000ull) >> 32);
    val = ((val & 0x0000FFFF0000FFFFull) << 16) | ((val & 0xFFFF0000FFFF0000ull) >> 16);
    val = ((val & 0x00FF00FF00FF00FFull) << 8)  | ((val & 0xFF00FF00FF00FF00ull) >> 8);
    return val;
}

typedef struct {
    uint32_t magic;
    int32_t  off_str;
    int32_t  off_dat;
    uint32_t n_files;
} __attribute__((packed)) pako_header_t;

typedef struct {
    int32_t off_file_name;
    uint32_t file_size;
    int32_t off_file_content;
    uint64_t check_sum;

} __attribute__((packed)) file_e;

string get_byte(int n){
    string result = "";
    while(n){
        int now = n & 15;
        n >>= 4;

        if(now >= 10) result.push_back((now-10) + 'A');
        else result.push_back(now + '0');
    }
    if(result == "") result = "0";
    reverse(result.begin(), result.end());

    return result;
}

int main(int argc, char* argv[]){

    char* pakFile = argv[1];
    string folder_str = argv[2];
    cout << pakFile << endl << folder_str << endl;

    int fd;
    fd = open(pakFile, O_RDONLY);

    pako_header_t header;
    read(fd, &header, 16);
    cout << header.magic << "  -- little endian(50414B4F -> 4F4B4150) number result" << endl;
    cout << header.off_str << endl;
    cout << header.off_dat << endl;
    cout << header.n_files << endl;
    // [1] pointer: header end

    vector<file_e> file_e_vec;
    file_e_vec.resize(header.n_files);
    for(int i = 0; i < header.n_files; i++){
        read(fd, &file_e_vec[i], 20);
        file_e_vec[i].file_size = SwapEndian_32(file_e_vec[i].file_size);
        file_e_vec[i].check_sum = SwapEndian_64(file_e_vec[i].check_sum);
    }
    // [2] pointer: file_e end

    vector<string> file_name;
    file_name.resize(header.n_files);
    for(int i = 0; i < header.n_files; i++){
        lseek(fd, header.off_str+file_e_vec[i].off_file_name, SEEK_SET);  // lseek the start of each name
        char tmp_buf[1];
        while(true){
            read(fd, tmp_buf, 1);
            if(tmp_buf[0] == '\0') break;
            file_name[i].push_back(tmp_buf[0]);
        }
    }

    for(int i = 0; i < header.n_files; i++){
        cout << "file " << i+1 << "\tname | size in byte | size:\t" << file_name[i] << "\t| " << get_byte(file_e_vec[i].file_size) << "\t| " << file_e_vec[i].file_size << endl;
    }
    // [3] pointer: string = file_name section end
    // check checksum
    // first get content and xor them
    // (overall_content_offset + single_content_offset) ~ (overall_content_offset + single_content_offset) + file_size
    cout << "file content range" << endl;
    for(int i = 0; i < header.n_files; i++){
        cout << "file " << i+1 << " : " << (header.off_dat + file_e_vec[i].off_file_content) << " ~ " <<(header.off_dat + file_e_vec[i].off_file_content) + file_e_vec[i].file_size << endl;
    }

    for(int i = 0; i < header.n_files; i++){
        lseek(fd, header.off_dat + file_e_vec[i].off_file_content, SEEK_SET);  // move the pointer to the start of the each file content

        // check buf's check sum
        // if(checksum(buf, extend_file_size, file_e_vec[i].check_sum) == false) continue;
        int split_number = ((file_e_vec[i].file_size)%8 > 0)? (file_e_vec[i].file_size/8+1) : (file_e_vec[i].file_size/8);
        vector<uint64_t> split_content;
        split_content.resize(split_number);
        uint64_t xor_result = 0; // 0 與大家 xor 會是大家
        for(int j = 0; j < split_number; j++){
            read(fd, &split_content[j], 8);
            xor_result = xor_result ^ split_content[j];
            // cout << split_content[j] << endl;
        }
        cout << xor_result << " | " << file_e_vec[i].check_sum << endl;

        if( xor_result != file_e_vec[i].check_sum) continue;

        string path = folder_str + "/" + file_name[i];
        char* path_ptr = path.data();
        cout << path_ptr << endl;

        char *content_buf = (char*)calloc(file_e_vec[i].file_size, sizeof(char));
        lseek(fd, header.off_dat + file_e_vec[i].off_file_content, SEEK_SET); 
        read(fd, content_buf, file_e_vec[i].file_size);

        cout << "content: " << content_buf << " | filesize: " << file_e_vec[i].file_size << endl;

        int tmp_fd;
        tmp_fd = open(path_ptr, O_CREAT|O_RDWR, S_IRUSR);  // not really sure the flag and mode
        int ret = write(tmp_fd, content_buf, file_e_vec[i].file_size);
        cout << "tmp_fd | ret value: " << tmp_fd << " | " << ret << endl;
        close(tmp_fd);
    }

    close(fd);

    return 0;
}