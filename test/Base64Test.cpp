#define BOOST_TEST_MODULE Base64
#include <boost/test/unit_test.hpp>

#include "base64.h"

#include <vector>

BOOST_AUTO_TEST_SUITE( Base64Test )

BOOST_AUTO_TEST_CASE( encode_empty )
{
    std::vector<uint8_t> text = {};
    BOOST_CHECK( encode(text) == "" );
}

BOOST_AUTO_TEST_CASE( encode_one )
{
    std::vector<uint8_t> text = { '1' };
    std::string textEncoded = encode(text);
    BOOST_CHECK_MESSAGE( textEncoded == "MQ==", textEncoded + " is not equal MQ==" );
}

BOOST_AUTO_TEST_CASE( encode_two )
{
    std::vector<uint8_t> text = { '1', '2' };
    std::string textEncoded = encode(text);
    BOOST_CHECK_MESSAGE( textEncoded == "MTI=", textEncoded + " is not equal MTI=" );
}

BOOST_AUTO_TEST_CASE( encode_three )
{
    std::vector<uint8_t> text = { '1', '2', '3' };
    std::string textEncoded = encode(text);
    BOOST_CHECK_MESSAGE( textEncoded == "MTIz", textEncoded + " is not equal MTIz" );
}

BOOST_AUTO_TEST_CASE( encode_decode_empty )
{
    std::vector<uint8_t> text = {};
    BOOST_CHECK( decode(encode(text)) == text );
}

BOOST_AUTO_TEST_CASE( encode_decode_one )
{
    std::vector<uint8_t> text = { '1' };
    BOOST_CHECK( decode(encode(text)) == text );
}

BOOST_AUTO_TEST_CASE( encode_decode_two )
{
    std::vector<uint8_t> text = { '1', '2' };
    BOOST_CHECK( decode(encode(text)) == text );
}

BOOST_AUTO_TEST_CASE( encode_decode_three )
{
    std::vector<uint8_t> text = { '1', '2', '3' };
    BOOST_CHECK( decode(encode(text)) == text );
}

BOOST_AUTO_TEST_SUITE_END()