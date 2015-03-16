#include <gmock/gmock.h>
#include <memory>
#include <msf.h>

class parce_pdb_test: public testing::Test
{
public:
};

TEST_F(parce_pdb_test, openclose)
{
     std::shared_ptr<symenglib::MSF> msf(new symenglib::MSF());
     msf->Open("testexe1.pdb");
     msf->Close();
} 
 
