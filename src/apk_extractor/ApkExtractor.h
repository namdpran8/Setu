#pragma once
#include "miniz.h"
#include <string>
#include <vector>
#include <cstdint>

struct ApkEntry {
	std::string filename;
	std::uint64_t uncompressedSize;
	std::uint64_t compressedSize;
	bool isCompressed;

};



class ApkExtractor {
public:
	ApkExtractor();
	~ApkExtractor();



	bool OpenApk(const std::string& apkPath);
	void closeApk();

	std::vector<ApkEntry> listEntries() const;
	bool ExtractEntry(const std::string& entryName, const std::string& outputPath) const;
	void extractor();

	// New: Extract directly to memory for parsing
	bool ExtractEntryToMemory(const std::string& entryName, std::vector<uint8_t>& outBuffer) const;
	
private:
	mz_zip_archive* m_zipArchive;
	bool m_isApkOpen;
};