#include "ext2fs.h"
#include <bitset>
#include <cmath>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <vector>

class ext2 {
public:
    ext2(int fd) : file_desc(fd) {}

    bool isPath(std::string source);

    void read_super_block_data(ext2_super_block &super_block);

    int find_group_no(int group_no);

    int file_desc;
    int block_size;
    int inode_size;
    int inode_per_bg;
    ext2_block_group_descriptor source_block;
    int source_block_no;
    int inode_count;
    int block_per_bg;
    int total_blocks;

    int find_inode_of_source(std::string source);

    int find_dir_inode(ext2_inode ext_2_inode, std::string dir_name);

    int find_inode_of_dest(std::string source, std::string &out);

    int find_free_inode();

    int find_free_block();

    void update_image_file(int inode_no, int source_inode);

    int copy_metadata(int si, int fi);

    void insert_file(std::string file_name, int dest_inode, int free_inode);

    void find_target_entry(std::string file_name, int dest_inode,
                           int &inode_file);

    void read_block_group_data(int file_desc, int block_group,
                               ext2_block_group_descriptor &ext2_bgd,
                               int block_size);

    bool delete_inode(int inode_file);

    void decrease_ref_count(int inode_file);
};

bool ext2::isPath(std::string source) {
    bool is = '/' == source.at(0);
    return is;
}

void ext2::read_super_block_data(ext2_super_block &super_block) {

    lseek(file_desc, 1024, SEEK_SET);
    read(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));
    block_size = 1024 << super_block.log_block_size;
    inode_size = super_block.inode_size;
}

void ext2::read_block_group_data(int file_desc, int block_group,
                                 ext2_block_group_descriptor &ext2_bgd,
                                 int block_size) {
    if (block_size == 1024) {
        block_size = 2048;
    }
    lseek(file_desc, block_group * sizeof(struct ext2_block_group_descriptor) + block_size, SEEK_SET);
    read(file_desc, (void *) (&ext2_bgd),
         sizeof(struct ext2_block_group_descriptor));
}


int ext2::find_free_inode() {

    ext2_block_group_descriptor bgd;

    int inode = 1;
    int current_group = 0;
    uint8_t inode_bitmap[block_size];

    int total_bg = inode_count / inode_per_bg;

    while (current_group <= total_bg) {
        if (block_size == 1024) {
            lseek(file_desc, 2048 + current_group * sizeof(ext2_block_group_descriptor), SEEK_SET);
        } else {
            lseek(file_desc, block_size + current_group * sizeof(ext2_block_group_descriptor), SEEK_SET);
        }

        read(file_desc, &bgd, sizeof(struct ext2_block_group_descriptor));

        lseek(file_desc, bgd.inode_bitmap * block_size, SEEK_SET);

        for (int i = 0; i < block_size; i++) {
            read(file_desc, &inode_bitmap[i], sizeof(uint8_t));
        }
        std::bitset<8> bs[block_size];
        for (int i = 0; i < block_size; i++) {
            bs[i] = inode_bitmap[i];
        }

        while (inode < inode_per_bg) {

            int bitmap_index = ((inode - 1) % inode_per_bg) / (8 * sizeof(uint8_t));
            int bitmap_location =
                    ((inode - 1) % inode_per_bg) % (8 * sizeof(uint8_t));
            if (!(bs[bitmap_index][bitmap_location])) {
                return inode;
            }
            inode += 1;
        }
        current_group += 1;
    }
    return 0;
}

int ext2::find_free_block() {

    ext2_block_group_descriptor bgd;

    int block = 0;
    int current_group = 0;
    uint8_t block_bitmap[block_size];

    int total_bg = total_blocks / block_per_bg;

    while (current_group <= total_bg) {
        if (block_size == 1024) {
            lseek(file_desc, 2048 + current_group * sizeof(ext2_block_group_descriptor), SEEK_SET);
        } else {
            lseek(file_desc, block_size + current_group * sizeof(ext2_block_group_descriptor), SEEK_SET);
        }

        read(file_desc, &bgd, sizeof(struct ext2_block_group_descriptor));

        lseek(file_desc, bgd.block_bitmap * block_size, SEEK_SET);

        for (int i = 0; i < block_size; i++) {
            read(file_desc, &block_bitmap[i], sizeof(uint8_t));
        }
        std::bitset<8> bs[block_size];
        for (int i = 0; i < block_size; i++) {
            bs[i] = block_bitmap[i];
        }

        while (block < block_per_bg) {

            int bitmap_index = ((block)) / (8 * sizeof(uint8_t));
            int bitmap_location = ((block)) % (8 * sizeof(uint8_t));
            if (!(bs[bitmap_index][bitmap_location])) {
                return block;
            }
            block += 1;
        }
        current_group += 1;
    }
    return 0;
}

void ext2::update_image_file(int inode_no, int source_inode) {
    ext2_super_block super_block;
    lseek(file_desc, 1024, SEEK_SET);
    read(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));

    super_block.free_inode_count -= 1;
    lseek(file_desc, 1024, SEEK_SET);
    write(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));

    ext2_block_group_descriptor bgd;
    int group_no = (inode_no - 1) / inode_per_bg;
    int place = block_size + group_no * sizeof(ext2_block_group_descriptor);
    if (block_size == 1024) {
        place += 1024;
    }

    lseek(file_desc, place, SEEK_SET);
    read(file_desc, (void *) (&bgd), sizeof(struct ext2_block_group_descriptor));

    bgd.free_inode_count -= 1;
    lseek(file_desc, place, SEEK_SET);
    write(file_desc, (void *) (&bgd), sizeof(struct ext2_block_group_descriptor));

    uint8_t inode_bitmap[block_size];

    lseek(file_desc, bgd.inode_bitmap * block_size, SEEK_SET);

    for (int i = 0; i < block_size; i++) {
        read(file_desc, &inode_bitmap[i], sizeof(uint8_t));
    }
    std::bitset<8> bs[block_size];
    for (int i = 0; i < block_size; i++) {
        bs[i] = inode_bitmap[i];
    }

    int bitmap_index = ((inode_no - 1) % inode_per_bg) / (8 * sizeof(uint8_t));
    int bitmap_location = ((inode_no - 1) % inode_per_bg) % (8 * sizeof(uint8_t));
    uint8_t inode_bm_after = 0;
    bs[bitmap_index][bitmap_location] = 1;

    for (int i = 0; i < 8; i++) {
        inode_bm_after += pow(2, i) * bs[bitmap_index][i];
    }

    inode_bitmap[bitmap_index] = inode_bm_after;
    lseek(file_desc, bgd.inode_bitmap * block_size, SEEK_SET);

    for (int i = 0; i < block_size; i++) {
        write(file_desc, (void *) (&inode_bitmap[i]), sizeof(uint8_t));
    }

    ext2_inode ext2_i;
    int index = (source_inode - 1) % inode_per_bg;
    int containing_block = (index * inode_size);
    lseek(file_desc, block_size * (bgd.inode_table) + containing_block, SEEK_SET);
    read(file_desc, &ext2_i, sizeof(ext2_inode));
    std::vector<int> block_numbers;
    for (int i = 0; i < EXT2_NUM_DIRECT_BLOCKS; i++) {
        if (ext2_i.direct_blocks[i] != 0) {
            block_numbers.push_back(ext2_i.direct_blocks[i]);
        }
    }

    uint32_t fb;
    if (block_size == 1024) {
        for (int i = 0; i < block_numbers.size(); i++) {
            lseek(file_desc,
                  (bgd.block_refmap * block_size) + 4 * (block_numbers[i] - 1),
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));
            fb = fb + 1;
            lseek(file_desc,
                  (bgd.block_refmap * block_size) + 4 * (block_numbers[i] - 1),
                  SEEK_SET);
            write(file_desc, (&fb), sizeof(uint32_t));
        }
    } else {

        for (int i = 0; i < block_numbers.size(); i++) {
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * block_numbers[i],
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));
            fb = fb + 1;
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * block_numbers[i],
                  SEEK_SET);
            write(file_desc, (&fb), sizeof(uint32_t));
        }
    }
}

int ext2::copy_metadata(int si, int fi) {

    ext2_inode inode_source;
    ext2_block_group_descriptor bgd_source;
    int block_group = (si - 1) / inode_per_bg;
    read_block_group_data(file_desc, block_group, bgd_source, block_size);
    int block_index = (si - 1) % inode_per_bg;

    int containing_block = (block_index * inode_size);
    lseek(file_desc, block_size * (bgd_source.inode_table) + containing_block,
          SEEK_SET);
    read(file_desc, &inode_source, sizeof(inode_source));

    //inode_source.link_count += 1;
    //lseek(file_desc, block_size * (bgd_source.inode_table) + containing_block, SEEK_SET);
    //write(file_desc, &inode_source, sizeof(inode_source));

    ext2_inode inode_dest;
    ext2_block_group_descriptor bgd_dest;
    block_group = (fi - 1) / inode_per_bg;
    read_block_group_data(file_desc, block_group, bgd_dest, block_size);
    block_index = (fi - 1) % inode_per_bg;

    containing_block = (block_index * inode_size);
    lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
          SEEK_SET);
    read(file_desc, &inode_dest, sizeof(inode_dest));

    inode_dest.size = inode_source.size;
    inode_dest.gid = inode_source.gid;
    inode_dest.uid = inode_source.uid;
    inode_dest.creation_time = std::time(0);
    inode_dest.access_time = std::time(0);
    inode_dest.link_count = 1;
    inode_dest.modification_time = std::time(0);
    inode_dest.mode = inode_source.mode;
    inode_dest.block_count_512 = inode_source.block_count_512;
    inode_dest.deletion_time = 0;
    inode_dest.flags = inode_source.flags;
    inode_dest.double_indirect = inode_source.double_indirect;
    inode_dest.single_indirect = inode_source.single_indirect;
    inode_dest.triple_indirect = inode_source.triple_indirect;
    for (int i = 0; i < EXT2_NUM_DIRECT_BLOCKS; i++) {
        inode_dest.direct_blocks[i] = inode_source.direct_blocks[i];
    }

    lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
          SEEK_SET);
    write(file_desc, &inode_dest, sizeof(inode_dest));

    return inode_source.size;
}

int ext2::find_dir_inode(ext2_inode ext_2_inode, std::string dir_name) {
    int inode = -1;

    bool found_inode = false;
    for (int j = 0; j < EXT2_NUM_DIRECT_BLOCKS; j++) {
        int block_traverse = 0;

        if (ext_2_inode.direct_blocks[j] != 0) {
            while (block_traverse < block_size && found_inode == false) {
                char *name = new char[255]();
                bool isMatch = false;
                int offset = ext_2_inode.direct_blocks[j] * block_size + block_traverse;
                lseek(file_desc, offset, SEEK_SET);
                ext2_dir_entry dir;

                read(file_desc, &(dir.inode), sizeof(uint32_t));
                read(file_desc, &(dir.length), sizeof(uint16_t));
                read(file_desc, &(dir.name_length), sizeof(uint8_t));
                read(file_desc, &(dir.file_type), sizeof(uint8_t));
                read(file_desc, name, int(dir.name_length));

                if (dir.name_length == dir_name.size()) {
                    int match = 0;
                    for (int i = 0; i < dir_name.size(); i++) {
                        if (dir_name[i] == name[i]) {
                            match++;
                        }
                    }
                    if (match == dir.name_length) {
                        isMatch = true;
                    }
                }

                if (isMatch) {
                    inode = dir.inode;
                    found_inode = true;
                }
                block_traverse += dir.length;
                delete[] name;
            }
            if (found_inode) {
                break;
            }
        }
    }

    return inode;
}

int ext2::find_inode_of_source(std::string source) {
    int source_inode = -1;
    bool is_path_source = isPath(source);
    if (!is_path_source) {
        source_inode = stoi(source);

    } else {
        std::vector <std::string> substrings;
        int prev = 1;
        for (int i = 1; i < source.size(); i++) {
            if (source[i] == '/') {
                substrings.push_back(source.substr(prev, i - prev));
                prev = i + 1;
            }
        }
        substrings.push_back(source.substr(prev, source.size()));

        int root_inode = 2;

        for (int i = 0; i < substrings.size(); i++) {

            ext2_block_group_descriptor bgd;
            ext2_inode ext2_i;
            int inode = root_inode;
            int block_group = (inode - 1) / inode_per_bg;
            read_block_group_data(file_desc, block_group, bgd, block_size);
            int block_index = (inode - 1) % inode_per_bg;

            int containing_block = (block_index * inode_size);
            lseek(file_desc, block_size * (bgd.inode_table) + containing_block,
                  SEEK_SET);
            read(file_desc, &ext2_i, sizeof(ext2_i));
            root_inode = find_dir_inode(ext2_i, substrings[i]);
        }

        source_inode = root_inode;
    }

    return source_inode;
}

int ext2::find_inode_of_dest(std::string source, std::string &dest_file) {
    int dest_inode = -1;
    bool is_path_dest = isPath(source);
    if (!is_path_dest) {
        std::string substring;
        int prev = 0;
        for (int i = 0; i < source.size(); i++) {
            if (source[i] == '/') {
                substring = source.substr(prev, i - prev);
                prev = i + 1;
            }
        }
        dest_inode = std::stoi(substring);
        dest_file = source.substr(prev, source.size());
    } else {
        std::vector <std::string> substrings;
        int prev = 1;
        for (int i = 1; i < source.size(); i++) {
            if (source[i] == '/') {
                substrings.push_back(source.substr(prev, i - prev));
                prev = i + 1;
            }
        }
        int root_inode = 2;
        for (int i = 0; i < substrings.size(); i++) {
            ext2_block_group_descriptor bgd;
            ext2_inode ext2_i;
            int inode = root_inode;
            int block_group = (inode - 1) / inode_per_bg;
            read_block_group_data(file_desc, block_group, bgd, block_size);
            int block_index = (inode - 1) % inode_per_bg;

            int containing_block = (block_index * inode_size);
            lseek(file_desc, block_size * (bgd.inode_table) + containing_block,
                  SEEK_SET);
            read(file_desc, &ext2_i, sizeof(ext2_i));
            root_inode = find_dir_inode(ext2_i, substrings[i]);
        }
        dest_inode = root_inode;
        dest_file = source.substr(prev, source.size());
    }
    return dest_inode;
}

void ext2::insert_file(std::string file_name, int dest_inode, int free_inode) {
    int dir_size = 0;
    int file_name_size = 0;
    file_name_size = file_name.length();
    int fs = file_name_size;
    fs += (fs % 4 != 0 ? (4 - fs % 4) : 0);
    dir_size = fs + 8;

    ext2_inode inode_dest;
    ext2_block_group_descriptor bgd_dest;
    int block_group = (dest_inode - 1) / inode_per_bg;
    read_block_group_data(file_desc, block_group, bgd_dest, block_size);
    int block_index = (dest_inode - 1) % inode_per_bg;

    int containing_block = (block_index * inode_size);
    lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
          SEEK_SET);
    read(file_desc, &inode_dest, sizeof(inode_dest));

    bool found = false;
    for (int j = 0; j < EXT2_NUM_DIRECT_BLOCKS; j++) {
        int block_traverse = 0;
        int size_left = block_size;

        if (inode_dest.direct_blocks[j] != 0) {
            while (block_traverse < block_size && !found) {
                int offset = inode_dest.direct_blocks[j] * block_size;
                lseek(file_desc, offset + block_traverse, SEEK_SET);
                ext2_dir_entry dir;

                read(file_desc, &(dir.inode), sizeof(uint32_t));
                read(file_desc, &(dir.length), sizeof(uint16_t));
                read(file_desc, &(dir.name_length), sizeof(uint8_t));

                uint16_t needed_size = dir.name_length + 8;
                needed_size +=
                        (dir.name_length % 4 != 0 ? (4 - dir.name_length % 4) : 0);

                if (dir.inode == 0 && dir_size <= dir.length) {
                    // allocate_block_in_rem_one();
                    lseek(file_desc, offset + block_traverse, SEEK_SET);
                    write(file_desc, &(free_inode), sizeof(uint32_t));
                    write(file_desc, &(dir.length), sizeof(uint16_t));
                    write(file_desc, &(file_name_size), sizeof(uint8_t));
                    uint8_t ft = 1;
                    write(file_desc, &(ft), sizeof(uint8_t));
                    char *char_ptr = new char[file_name.size() + 1]();

                    std::copy(file_name.begin(), file_name.end(), char_ptr);
                    char_ptr[file_name.size()] = '\0';

                    write(file_desc, char_ptr, file_name.size() + 1);
                    delete[] char_ptr;

                    found = true;
                } else if (needed_size < dir.length && size_left == dir.length &&
                           (needed_size + dir_size) <= size_left) {

                    lseek(file_desc, offset + (block_traverse + 4), SEEK_SET);
                    write(file_desc, &needed_size, sizeof(uint16_t));
                    size_left -= needed_size;
                    block_traverse += needed_size;
                    lseek(file_desc, offset + block_traverse, SEEK_SET);
                    write(file_desc, &(free_inode), sizeof(uint32_t));
                    write(file_desc, &(size_left), sizeof(uint16_t));
                    write(file_desc, &(file_name_size), sizeof(uint8_t));
                    uint8_t ft = 1;
                    write(file_desc, &(ft), sizeof(uint8_t));

                    char *char_ptr = new char[file_name.size() + 1]();

                    std::copy(file_name.begin(), file_name.end(), char_ptr);
                    char_ptr[file_name.size()] = '\0';

                    write(file_desc, char_ptr, file_name.size() + 1);
                    delete[] char_ptr;

                    found = true;
                }
                size_left -= dir.length;
                block_traverse += dir.length;
            }

            if (found) {
                std::cout << "-1" << std::endl;
                break;
            }
        } else {

            uint32_t free_block = find_free_block();
            std::cout << free_block << std::endl;
            lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
                  SEEK_SET);
            read(file_desc, &inode_dest, sizeof(inode_dest));
            inode_dest.direct_blocks[j] = free_block;
            lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
                  SEEK_SET);
            write(file_desc, &inode_dest, sizeof(ext2_inode));

            int offset = inode_dest.direct_blocks[j] * block_size;
            lseek(file_desc, offset, SEEK_SET);
            write(file_desc, &(free_inode), sizeof(uint32_t));
            write(file_desc, &(block_size), sizeof(uint16_t));
            write(file_desc, &(file_name_size), sizeof(uint8_t));
            uint8_t ft = 1;
            write(file_desc, &(ft), sizeof(uint8_t));
            char *char_ptr = new char[file_name.size() + 1]();

            std::copy(file_name.begin(), file_name.end(), char_ptr);
            char_ptr[file_name.size()] = '\0';

            write(file_desc, char_ptr, file_name.size() + 1);
            delete[] char_ptr;

            uint8_t block_bitmap[block_size];

            lseek(file_desc, bgd_dest.block_bitmap * block_size, SEEK_SET);

            for (int i = 0; i < block_size; i++) {
                read(file_desc, &block_bitmap[i], sizeof(uint8_t));
            }
            std::bitset<8> bs[block_size];
            for (int i = 0; i < block_size; i++) {
                bs[i] = block_bitmap[i];
            }

            int bitmap_index = (free_block) / (8 * sizeof(uint8_t));
            int bitmap_location = (free_block) % (8 * sizeof(uint8_t));

            if (block_size == 1024) {
                bitmap_index = (free_block - 1) / (8 * sizeof(uint8_t));
                bitmap_location = (free_block - 1) % (8 * sizeof(uint8_t));
            }


            bs[bitmap_index][bitmap_location] = 1;
            uint8_t block_bm_after = 0;

            for (int j = 0; j < 8; j++) {
                block_bm_after += pow(2, j) * bs[bitmap_index][j];
            }

            block_bitmap[bitmap_index] = block_bm_after;
            lseek(file_desc, bgd_dest.block_bitmap * block_size, SEEK_SET);

            for (int i = 0; i < block_size; i++) {
                write(file_desc, &(block_bitmap[i]), sizeof(uint8_t));
            }

            ext2_super_block super_block;
            lseek(file_desc, 1024, SEEK_SET);
            read(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));

            super_block.free_block_count -= 1;
            lseek(file_desc, 1024, SEEK_SET);
            write(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));

            ext2_block_group_descriptor bgd;
            int group_no = (dest_inode - 1) / inode_per_bg;
            int place = block_size + group_no * sizeof(ext2_block_group_descriptor);
            if (block_size == 1024) {
                place += 1024;
            }
            lseek(file_desc, place, SEEK_SET);
            read(file_desc, (void *) (&bgd),
                 sizeof(struct ext2_block_group_descriptor));

            bgd.free_block_count -= 1;
            lseek(file_desc, place, SEEK_SET);
            write(file_desc, (void *) (&bgd),
                  sizeof(struct ext2_block_group_descriptor));

            uint32_t fb = 0;
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * free_block,
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));
            fb = fb + 1;
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * free_block,
                  SEEK_SET);
            write(file_desc, (&fb), sizeof(uint32_t));

            break;
        }
    }
}

void ext2::find_target_entry(std::string file_name, int dest_inode,
                             int &inode_file) {

    ext2_inode inode_dest;
    ext2_block_group_descriptor bgd_dest;
    int block_group = (dest_inode - 1) / inode_per_bg;
    read_block_group_data(file_desc, block_group, bgd_dest, block_size);
    int block_index = (dest_inode - 1) % inode_per_bg;

    int containing_block = (block_index * inode_size);
    lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
          SEEK_SET);
    read(file_desc, &inode_dest, sizeof(inode_dest));
    std::vector <ext2_dir_entry> dirs[EXT2_NUM_DIRECT_BLOCKS];
    std::vector <std::string> name_vector[EXT2_NUM_DIRECT_BLOCKS];
    std::vector<int> traverse_vector[EXT2_NUM_DIRECT_BLOCKS];

    bool first_entry = false;
    bool last_entry = false;
    int found_index = -1;
    int found_place = -1;
    bool found = false;
    for (int j = 0; j < EXT2_NUM_DIRECT_BLOCKS; j++) {
        int block_traverse = 0;
        int counter = 0;
        if (inode_dest.direct_blocks[j] != 0) {

            while (block_traverse < block_size) {
                char *name = new char[500]();
                bool isMatch = false;
                lseek(file_desc,
                      inode_dest.direct_blocks[j] * block_size + block_traverse,
                      SEEK_SET);
                ext2_dir_entry dir;

                read(file_desc, &(dir.inode), sizeof(uint32_t));
                read(file_desc, &(dir.length), sizeof(uint16_t));
                read(file_desc, &(dir.name_length), sizeof(uint8_t));
                read(file_desc, &(dir.file_type), sizeof(uint8_t));
                read(file_desc, name, int(dir.name_length));

                if (dir.name_length == file_name.size()) {
                    int match = 0;
                    for (int i = 0; i < file_name.size(); i++) {
                        if (file_name[i] == name[i]) {
                            match++;
                        }
                    }
                    if (match == dir.name_length) {
                        isMatch = true;
                    }
                }
                if (isMatch) {
                    inode_file = dir.inode;
                    if (counter == 0) {
                        first_entry = true;
                    } else if (dir.length + block_traverse >= block_size) {
                        last_entry = true;
                    }
                    found_place = counter;
                    found_index = j;
                    found = true;
                }

                std::string str(name);

                name_vector[j].push_back(str);

                dirs[j].push_back(dir);

                traverse_vector[j].push_back(block_traverse);
                counter++;
                block_traverse += dir.length;

                delete[] name;
            }
        }
    }
    /*
  if(block_size == 1024){
    found_inde*x += 1;
  }*/

    if (found_place == 2) {
        first_entry = true;
    }
    if (first_entry) {

        lseek(file_desc,
              inode_dest.direct_blocks[found_index] * block_size +
              traverse_vector[found_index][found_place],
              SEEK_SET);
        uint32_t in = 0;
        write(file_desc, &(in), sizeof(uint32_t));
        write(file_desc, &(dirs[found_index][found_place].length),
              sizeof(uint16_t));
        write(file_desc, &(dirs[found_index][found_place].name_length),
              sizeof(uint8_t));
        write(file_desc, &(dirs[found_index][found_place].file_type),
              sizeof(uint8_t));

        char *char_ptr = new char[dirs[found_index][found_place].name_length + 1]();
        strcpy(char_ptr, name_vector[found_index][found_place].c_str());
        char_ptr[dirs[found_index][found_place].name_length] = '\0';

        write(file_desc, char_ptr, dirs[found_index][found_place].name_length + 1);
        delete[] char_ptr;
    } else if (last_entry) {

        for (int k = 0; k < traverse_vector[found_index].size(); k++) {

            if (k < found_place - 1) {
                lseek(file_desc,
                      inode_dest.direct_blocks[found_index] * block_size +
                      traverse_vector[found_index][k],
                      SEEK_SET);

                write(file_desc, &(dirs[found_index][k].inode), sizeof(uint32_t));
                write(file_desc, &(dirs[found_index][k].length), sizeof(uint16_t));
                write(file_desc, &(dirs[found_index][k].name_length), sizeof(uint8_t));
                write(file_desc, &(dirs[found_index][k].file_type), sizeof(uint8_t));

                char *char_ptr = new char[dirs[found_index][k].name_length + 1]();
                strcpy(char_ptr, name_vector[found_index][k].c_str());
                char_ptr[dirs[found_index][k].name_length] = '\0';

                write(file_desc, char_ptr, dirs[found_index][k].name_length + 1);
                delete[] char_ptr;

            } else if (k == found_place - 1) {
                lseek(file_desc,
                      inode_dest.direct_blocks[found_index] * block_size +
                      traverse_vector[found_index][k],
                      SEEK_SET);
                uint16_t len =
                        dirs[found_index][k].length + dirs[found_index][found_place].length;
                write(file_desc, &(dirs[found_index][k].inode), sizeof(uint32_t));
                write(file_desc, &(len), sizeof(uint16_t));
                write(file_desc, &(dirs[found_index][k].name_length), sizeof(uint8_t));
                write(file_desc, &(dirs[found_index][k].file_type), sizeof(uint8_t));
                char *char_ptr = new char[dirs[found_index][k].name_length + 1]();
                strcpy(char_ptr, name_vector[found_index][k].c_str());
                char_ptr[dirs[found_index][k].name_length] = '\0';

                write(file_desc, char_ptr, dirs[found_index][k].name_length + 1);
                delete[] char_ptr;

            }
        }

    } else {

        for (int k = 0; k < traverse_vector[found_index].size(); k++) {

            if (k < found_place) {
                lseek(file_desc,
                      inode_dest.direct_blocks[found_index] * block_size +
                      traverse_vector[found_index][k],
                      SEEK_SET);

                write(file_desc, &(dirs[found_index][k].inode), sizeof(uint32_t));
                write(file_desc, &(dirs[found_index][k].length), sizeof(uint16_t));
                write(file_desc, &(dirs[found_index][k].name_length), sizeof(uint8_t));
                write(file_desc, &(dirs[found_index][k].file_type), sizeof(uint8_t));

                char *char_ptr = new char[dirs[found_index][k].name_length + 1]();
                strcpy(char_ptr, name_vector[found_index][k].c_str());
                char_ptr[dirs[found_index][k].name_length] = '\0';

                write(file_desc, char_ptr, dirs[found_index][k].name_length + 1);
                delete[] char_ptr;

            } else if (k > found_place) {
                lseek(file_desc,
                      inode_dest.direct_blocks[found_index] * block_size +
                      traverse_vector[found_index][k] -
                      dirs[found_index][found_place].length,
                      SEEK_SET);
                if (k == traverse_vector[found_index].size() - 1) {
                    uint16_t len = dirs[found_index][k].length +
                                   dirs[found_index][found_place].length;
                    write(file_desc, &(dirs[found_index][k].inode), sizeof(uint32_t));
                    write(file_desc, &(len), sizeof(uint16_t));
                    write(file_desc, &(dirs[found_index][k].name_length),
                          sizeof(uint8_t));
                    write(file_desc, &(dirs[found_index][k].file_type), sizeof(uint8_t));
                    char *char_ptr = new char[dirs[found_index][k].name_length + 1]();
                    strcpy(char_ptr, name_vector[found_index][k].c_str());
                    char_ptr[dirs[found_index][k].name_length] = '\0';

                    write(file_desc, char_ptr,dirs[found_index][k].name_length + 1);
                    delete[] char_ptr;

                } else {
                    write(file_desc, &(dirs[found_index][k].inode), sizeof(uint32_t));
                    write(file_desc, &(dirs[found_index][k].length), sizeof(uint16_t));
                    write(file_desc, &(dirs[found_index][k].name_length),
                          sizeof(uint8_t));
                    write(file_desc, &(dirs[found_index][k].file_type), sizeof(uint8_t));
                    char *char_ptr = new char[dirs[found_index][k].name_length + 1]();
                    strcpy(char_ptr, name_vector[found_index][k].c_str());
                    char_ptr[dirs[found_index][k].name_length] = '\0';

                    write(file_desc, char_ptr, dirs[found_index][k].name_length + 1);
                    delete[] char_ptr;

                }
            }
        }
    }
}

bool ext2::delete_inode(int inode_file) {

    ext2_inode inode_dest;
    ext2_block_group_descriptor bgd_dest;
    int block_group = (inode_file - 1) / inode_per_bg;
    read_block_group_data(file_desc, block_group, bgd_dest, block_size);
    int block_index = (inode_file - 1) % inode_per_bg;

    int containing_block = (block_index * inode_size);
    lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
          SEEK_SET);

    read(file_desc, &inode_dest, sizeof(inode_dest));
    inode_dest.link_count -= 1;
    if (inode_dest.link_count == 0) {
        inode_dest.deletion_time = std::time(0);
    }
    lseek(file_desc, block_size * (bgd_dest.inode_table) + containing_block,
          SEEK_SET);
    write(file_desc, &inode_dest, sizeof(inode_dest));

    if (inode_dest.link_count == 0) {

        uint8_t inode_bitmap[block_size];

        lseek(file_desc, bgd_dest.inode_bitmap * block_size, SEEK_SET);

        for (int i = 0; i < block_size; i++) {
            read(file_desc, &inode_bitmap[i], sizeof(uint8_t));
        }
        std::bitset<8> bs[block_size];
        for (int i = 0; i < block_size; i++) {
            bs[i] = inode_bitmap[i];
        }

        int bitmap_index =
                ((inode_file - 1) % inode_per_bg) / (8 * sizeof(uint8_t));
        int bitmap_location =
                ((inode_file - 1) % inode_per_bg) % (8 * sizeof(uint8_t));
        uint8_t inode_bm_after = 0;

        bs[bitmap_index][bitmap_location] = 0;

        for (int i = 0; i < 8; i++) {
            inode_bm_after += pow(2, i) * bs[bitmap_index][i];
        }

        inode_bitmap[bitmap_index] = inode_bm_after;
        lseek(file_desc, bgd_dest.inode_bitmap * block_size, SEEK_SET);

        for (int i = 0; i < block_size; i++) {
            write(file_desc, &(inode_bitmap[i]), sizeof(uint8_t));
        }
    } else {
        return false;
    }
    return true;
}

void ext2::decrease_ref_count(int inode_no) {

    ext2_super_block super_block;
    lseek(file_desc, 1024, SEEK_SET);
    read(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));


    super_block.free_inode_count += 1;
    lseek(file_desc, 1024, SEEK_SET);
    write(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));

    ext2_block_group_descriptor bgd;
    int group_no = (inode_no - 1) / inode_per_bg;
    int place = block_size + group_no * block_size * block_per_bg;
    if (block_size == 1024) {
        place += 1024;
    }
    lseek(file_desc, place, SEEK_SET);
    read(file_desc, (void *) (&bgd), sizeof(struct ext2_block_group_descriptor));

    bgd.free_inode_count += 1;
    lseek(file_desc, place, SEEK_SET);
    write(file_desc, (void *) (&bgd), sizeof(struct ext2_block_group_descriptor));

    ext2_inode ext2_i;
    int index = (inode_no - 1) % inode_per_bg;
    int containing_block = (index * inode_size);


    lseek(file_desc, block_size * (bgd.inode_table) + containing_block, SEEK_SET);
    read(file_desc, &ext2_i, sizeof(ext2_inode));
    std::vector<int> block_numbers;
    for (int i = 0; i < EXT2_NUM_DIRECT_BLOCKS; i++) {
        if (ext2_i.direct_blocks[i] != 0) {
            block_numbers.push_back(ext2_i.direct_blocks[i]);
        }
    }
    ext2_block_group_descriptor new_bgd;

    int dest_block_g_no = -1;
    for (int i = 0; i < block_numbers.size(); i++) {
        dest_block_g_no = block_numbers[i] / block_per_bg;
        int place = block_size + dest_block_g_no * sizeof(ext2_block_group_descriptor);
        if (block_size == 1024) {
            place += 1024;
        }
        lseek(file_desc, place, SEEK_SET);

        read(file_desc, (void *) (&new_bgd), sizeof(struct ext2_block_group_descriptor));
    }


    bool block_de = false;

    uint32_t fb;
    if (block_size == 1024) {
        for (int i = 0; i < block_numbers.size(); i++) {
            fb = 0;
            int ind = (block_numbers[i]) % block_per_bg;
            lseek(file_desc,
                  ((new_bgd.block_refmap) * block_size + 4 * (ind - 1)),
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));


            fb = fb - 1;
            lseek(file_desc,
                  ((new_bgd.block_refmap) * block_size + 4 * (ind - 1)),
                  SEEK_SET);
            write(file_desc, (&fb), sizeof(uint32_t));
        }
        for (int i = 0; i < block_numbers.size(); i++) {
            int ind = (block_numbers[i]) % block_per_bg;
            lseek(file_desc,
                  ((new_bgd.block_refmap) * block_size + 4 * (ind - 1)),
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));
            if (fb == 0) {
                std::cout << block_numbers[i] << " ";
                block_de = true;
            }
        }
    } else {
        for (int i = 0; i < block_numbers.size(); i++) {
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * block_numbers[i],
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));
            fb = fb - 1;
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * block_numbers[i],
                  SEEK_SET);
            write(file_desc, (&fb), sizeof(uint32_t));
        }
        for (int i = 0; i < block_numbers.size(); i++) {
            lseek(file_desc, (bgd.block_refmap * block_size) + 4 * block_numbers[i],
                  SEEK_SET);
            read(file_desc, (&fb), sizeof(uint32_t));
            if (fb == 0) {
                std::cout << block_numbers[i] << " ";
                block_de = true;
            }
        }
    }
    if (block_de) {
        std::cout << std::endl;
        uint8_t block_bitmap[block_size];

        lseek(file_desc, new_bgd.block_bitmap * block_size, SEEK_SET);

        for (int i = 0; i < block_size; i++) {
            read(file_desc, &block_bitmap[i], sizeof(uint8_t));
        }
        std::bitset<8> bs[block_size];
        for (int i = 0; i < block_size; i++) {
            bs[i] = block_bitmap[i];
        }
        int bitmap_index[block_numbers.size()];
        int bitmap_location[block_numbers.size()];
        if (block_size == 1024) {
            for (int i = 0; i < block_numbers.size(); i++) {
                bitmap_index[i] = ((block_numbers[i] - 1) % block_per_bg) / (8 * sizeof(uint8_t));
                bitmap_location[i] = ((block_numbers[i] - 1) % block_per_bg) % (8 * sizeof(uint8_t));
            }
        } else {
            for (int i = 0; i < block_numbers.size(); i++) {
                bitmap_index[i] = ((block_numbers[i])) / (8 * sizeof(uint8_t));
                bitmap_location[i] = ((block_numbers[i])) % (8 * sizeof(uint8_t));
            }
        }


        for (int i = 0; i < block_numbers.size(); i++) {
            bs[bitmap_index[i]][bitmap_location[i]] = 0;
        }
        uint8_t block_bm_after[block_size];

        for (int i = 0; i < block_size; i++) {
            block_bm_after[i] = 0;
        }

        for (int i = 0; i < block_size; i++) {
            for (int j = 0; j < 8; j++) {
                block_bm_after[i] += pow(2, j) * bs[i][j];
            }
        }

        lseek(file_desc, new_bgd.block_bitmap * block_size, SEEK_SET);

        for (int i = 0; i < block_size; i++) {
            write(file_desc, &(block_bm_after[i]), sizeof(uint8_t));
        }

        super_block.free_block_count += block_numbers.size();
        lseek(file_desc, 1024, SEEK_SET);
        write(file_desc, (void *) (&super_block), sizeof(struct ext2_super_block));
        place = block_size + dest_block_g_no * (sizeof(ext2_block_group_descriptor));
        if (block_size == 1024) {
            place += 1024;
        }
        new_bgd.free_block_count += block_numbers.size();
        lseek(file_desc, place, SEEK_SET);
        write(file_desc, (void *) (&new_bgd),
              sizeof(struct ext2_block_group_descriptor));
    } else {
        std::cout << "-1" << std::endl;
    }
}

int main(int argc, char *argv[]) {

    int arg_count = argc;

    int file_desc = open(argv[2], O_RDWR);
    if (file_desc == -1) {
        std::perror("Failed opening img");
        return -1;
    }

    ext2 class_ext2(file_desc);

    ext2_super_block ext2_sb;
    class_ext2.read_super_block_data(ext2_sb);

    class_ext2.inode_per_bg = ext2_sb.inodes_per_group;
    class_ext2.inode_count = ext2_sb.inode_count;
    class_ext2.block_per_bg = ext2_sb.blocks_per_group;
    class_ext2.total_blocks = ext2_sb.block_count;
    ext2_inode ext2_i;

    if (arg_count == 5) {
        if ((strcmp(argv[1], "dup")) == 0) {
            ext2_block_group_descriptor ext2_bgd;
            ext2_dir_entry source_dir;
            ext2_dir_entry dest_dir;
            std::string last_dir;
            int source_inode = class_ext2.find_inode_of_source(argv[3]);
            int dest_inode = class_ext2.find_inode_of_dest(argv[4], last_dir);
            int free_inode = class_ext2.find_free_inode();
            std::cout << free_inode << std::endl;
            class_ext2.update_image_file(free_inode, source_inode);
            class_ext2.copy_metadata(source_inode, free_inode);
            class_ext2.insert_file(last_dir, dest_inode, free_inode);
        }
    } else if (arg_count == 4) {
        if ((strcmp(argv[1], "rm")) == 0) {
            std::string last_dir;
            int dest_inode = class_ext2.find_inode_of_dest(argv[3], last_dir);
            int inode_file = -1;
            class_ext2.find_target_entry(last_dir, dest_inode, inode_file);
            std::cout << inode_file << std::endl;
            bool ret_val = class_ext2.delete_inode(inode_file);
            if (ret_val) {
                class_ext2.decrease_ref_count(inode_file);
            } else {
                std::cout << "-1" << std::endl;
            }
        }
    }
    close(file_desc);
    return 0;
}
