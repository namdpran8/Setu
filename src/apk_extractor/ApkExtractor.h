/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

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