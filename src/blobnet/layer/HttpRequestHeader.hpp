#pragma once

#include <string>
#include <vector>

namespace BlobNet {
namespace Layer {

/// \class HttpRequestHeader
/// \brief Adds additional header information to an request
class HttpRequestHeader {
public:
	HttpRequestHeader() = default;
	
	/// adds an Basic Authentication header entry
	/// \param name: Username for authentication
	/// \param password: Password for authentication
	void addAuthorization(const std::string& name, const std::string& password);

	/// Returns the HttpRequestHeader information as string
	std::string getHeaderString() const;
private:
	std::vector<std::string> mHeaderEntries;
};

}
}
