
#include <gmock/gmock.h>



class first_test_fixture: public testing::Test
{
public:
};

TEST_F(first_test_fixture, firsttest1)
{
     ASSERT_TRUE(true);
    

} 
