#pragma once
#include <string>
#include <vector>
#include <cstdint>

struct ApkEntry {
	std::string Apkname;
	std::uint64_t UncompressedSize;
	std::uint64_t CompressedSize;
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

	
};