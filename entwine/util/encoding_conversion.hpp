/***************************************************************************
                           encoding_conversion.h
  ------------------------------------------------------------------------
  brief                :  brief
  date                 :  2025/04/23
  copyright            : (C) 2025 by XGEOS
 ***************************************************************************/

#pragma once

#include <string>
#include <stdexcept>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#else
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#endif

namespace entwine {
	namespace
	{
		// 获取本地编码
		std::string get_local_encoding() {
#ifdef _WIN32
			unsigned int cp = GetACP(); // 获取当前系统代码页
			return "CP" + std::to_string(cp);
#else
			setlocale(LC_ALL, ""); // 初始化 locale
			std::string encoding = nl_langinfo(CODESET); // 获取当前 locale 的编码
			return encoding.empty() ? "UTF-8" : encoding; // 默认为 UTF-8
#endif
		}

		// 本地编码转 UTF-8
		std::string local_to_utf8(const std::string& local_str, const std::string& local_encoding = "") {
#ifdef _WIN32
			// Windows 实现
			unsigned int codepage = 0;
			if (local_encoding.empty()) {
				codepage = GetACP(); // 默认使用系统代码页
			}
			else if (local_encoding.find("CP") == 0) {
				try {
					codepage = std::stoi(local_encoding.substr(2)); // 提取代码页
				}
				catch (...) {
					throw std::runtime_error("Invalid codepage format: " + local_encoding);
				}
			}
			else {
				throw std::runtime_error("Unsupported encoding format on Windows: " + local_encoding);
			}

			// 1. 本地编码转 Unicode
			int wstr_len = MultiByteToWideChar(codepage, 0, local_str.c_str(), -1, nullptr, 0);
			if (wstr_len == 0) {
				throw std::runtime_error("Local codepage to Unicode conversion failed");
			}
			std::wstring wstr(wstr_len, 0);
			MultiByteToWideChar(codepage, 0, local_str.c_str(), -1, &wstr[0], wstr_len);

			// 2. Unicode 转 UTF-8
			int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
			if (utf8_len == 0) {
				throw std::runtime_error("Unicode to UTF-8 conversion failed");
			}
			std::string utf8_str(utf8_len, 0);
			WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &utf8_str[0], utf8_len, nullptr, nullptr);

			return utf8_str.substr(0, utf8_len - 1);
#else
			// Linux/macOS 实现（使用 iconv）
			std::string source_encoding = local_encoding.empty() ? get_local_encoding() : local_encoding;

			iconv_t cd = iconv_open("UTF-8", source_encoding.c_str());
			if (cd == (iconv_t)-1) {
				throw std::runtime_error("iconv_open failed for " + source_encoding + " to UTF-8");
			}

			size_t in_bytes = local_str.size();
			size_t out_bytes = in_bytes * 4; // 分配足够大的输出缓冲区
			std::string out_buf(out_bytes, 0);
			char* in_ptr = const_cast<char*>(local_str.data());
			char* out_ptr = &out_buf[0];
			size_t out_bytes_left = out_bytes;

			size_t result = iconv(cd, &in_ptr, &in_bytes, &out_ptr, &out_bytes_left);
			iconv_close(cd);

			if (result == (size_t)-1) {
				throw std::runtime_error("iconv conversion failed from " + source_encoding + " to UTF-8");
			}

			return out_buf.substr(0, out_bytes - out_bytes_left);
#endif
		}


	}
}