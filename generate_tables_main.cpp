#include "bitboard_tables.h"
#include <fstream>
#include <iostream>
#include <string>

int main() {
    // 生成预计算表
    generate_tables();
    
    std::cout << "Tables generated successfully!" << std::endl;
    return 0;
}
