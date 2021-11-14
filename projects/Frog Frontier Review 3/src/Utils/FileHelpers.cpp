#include "Utils/FileHelpers.h"
#include <fstream>
#include <filesystem>
#include <Logging.h>

#include "Utils/StringUtils.h"

std::string FileHelpers::ReadFile(const std::string& filename) {
	std::string result;
	std::ifstream in(filename, std::ios::in | std::ios::binary); // ifstream closes itself due to RAII

	if (in) {
		// Determine how long the file is
		in.seekg(0, std::ios::end);
		size_t size = in.tellg();

		// As long as we have a size, we can read it!
		if (size != -1) {
			// Resize our string to be able to fit all the data
			result.resize(size);
			// Seek to beginning of file, then read contents to string
			in.seekg(0, std::ios::beg);
			in.read(&result[0], size);
		} else {
			LOG_ERROR("Could not read from file '{}'", filename);
		}
	} else {
		LOG_ERROR("Could not open file '{}'", filename);
	}

	return result;
}

std::string FileHelpers::ReadResolveIncludes(const std::string& filename) {
	// Read the entire file contents for processing
	std::string result = ReadFile(filename);
	// Determine where the file we just read resides on the filesystem
	const std::filesystem::path folder = std::filesystem::path(filename).parent_path();

	// The token we're looking for, and it's length
	const char* includeToken = "#include";
	const size_t includeTokenLen = const_strlen(includeToken);

	// Look for the token in the file
	size_t seek = result.find(includeToken, 0);
	// If we found it, there's work to do!
	while (seek != std::string::npos) {
		// Find the end of the line
		size_t eol = result.find_first_of("\r\n", seek);
		LOG_ASSERT(eol != std::string::npos, "Syntax error, no eol found after type token");

		// Calculate the area from end of token to end of line, snip out as the path
		size_t begin = seek + includeTokenLen + 1;
		std::string path = result.substr(begin, eol - begin);
		// Trim whitespace and any quotes 
		StringTools::Trim(path);
		StringTools::Trim(path, '"');
		// Determine the file path
		std::filesystem::path target;
		// If it starts with '/', relative to application directory
		if (path[0] == '/') {
			target = path;
		}
		// Otherwise relative to the current directory
		else {
			target = folder / path;
		}

		// Make sure file exists, then load and resolve it's includes
		LOG_ASSERT(std::filesystem::exists(target), "File does not exist");
		std::string replacement = FileHelpers::ReadResolveIncludes(target.string());

		// Inject result into our string
		result.replace(seek, eol - seek, replacement);
		// Look for more includes!
		seek = result.find(includeToken, seek + replacement.length());
	}

	return result;
}

void FileHelpers::WriteContentsToFile(const std::string& filename, const std::string& contents, bool append /*= false*/) {
	std::ofstream output(filename, std::ios::out | (append ? std::ios::app : 0));
	output << contents;
}
