#include <iostream>
#include "apk_extractor/apkextractor.h"



int main() {
    std::cout << "winandroid runtime - hello" << std::endl;
	ApkExtractor extract;
	extract.extractor();

    return 0;
}