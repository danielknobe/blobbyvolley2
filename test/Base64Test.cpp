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
    BOOST_CHECK( encode(text) == "MQ==" );
}

BOOST_AUTO_TEST_CASE( encode_two )
{
    std::vector<uint8_t> text = { '1', '2' };
    BOOST_CHECK( encode(text) == "MTI=" );
}

BOOST_AUTO_TEST_CASE( encode_three )
{
    std::vector<uint8_t> text = { '1', '2', '3' };
    BOOST_CHECK( encode(text) == "MTIz" );
}
#include <iostream>
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