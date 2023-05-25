// Implements method which is required for linking with CppAD.
// Probably the problem is due to some mistake in the current CppAD implementation.

// Author: Aliaksandr Nekrashevich
// Email: aliaksandr.nekrashevich@queensu.ca
// (c) Smith School of Business, 2023

# include <vector>
# include <stdio.h>
# include <stdlib.h>
# include <unistd.h>
# include <cppad/configure.hpp>
# include <cppad/local/temp_file.hpp>
# include <filesystem>


namespace CppAD {
namespace local {

using std::filesystem::path;

std::string temp_file(void) {
   path tmpDirPath = std::filesystem::temp_directory_path();

   std::string tmpDirStr = tmpDirPath.string();
   const char separator = '/';
   if (tmpDirStr.back() != separator) {
      tmpDirStr += separator;
   }

   std::string patternStr = tmpDirStr;
   patternStr += "fileXXXXXX";

   std::vector<char> patternVec( patternStr.size() + 1 );
   for(size_t idx = 0; idx < patternStr.size(); ++idx)
      patternVec[idx] = patternStr[idx];
   patternVec[patternStr.size()] = '\0';

   int fileDescriptor = mkstemp(patternVec.data());
   if (fileDescriptor < 0) {
      return "";
   }
   close(fileDescriptor);

   std::string fileName = patternVec.data();
   return fileName;
}

}
}


