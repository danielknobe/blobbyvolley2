#include "HttpRequestHeader.hpp"
#include "../../base64.h"

#include <numeric>

namespace BlobNet {
namespace Layer {

void HttpRequestHeader::addAuthorization(const std::string& name, const std::string& password)
{
	std::string const authorization = name + ":" + password;
	mHeaderEntries.push_back("Authorization: Basic " + encode(authorization.c_str(), authorization.c_str() + authorization.length()));
}

std::string HttpRequestHeader::getHeaderString() const
{
	return std::accumulate(mHeaderEntries.begin(), 
	                       mHeaderEntries.end(), 
	                       std::string(), 
	                       [](std::string acc, const std::string& current) { return acc + current + "\r\n"; });
}

}
}