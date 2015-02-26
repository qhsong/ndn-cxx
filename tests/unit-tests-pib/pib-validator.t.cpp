/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2013-2015 Regents of the University of California.
 *
 * This file is part of ndn-cxx library (NDN C++ library with eXperimental eXtensions).
 *
 * ndn-cxx library is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free Software
 * Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * ndn-cxx library is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
 * PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.
 *
 * You should have received copies of the GNU General Public License and GNU Lesser
 * General Public License along with ndn-cxx, e.g., in COPYING.md file.  If not, see
 * <http://www.gnu.org/licenses/>.
 *
 * See AUTHORS.md for complete list of ndn-cxx authors and contributors.
 */

#include "pib/pib-validator.hpp"
#include "security/pib/update-param.hpp"
#include "security/pib/delete-param.hpp"
#include "security/key-chain.hpp"

#include "boost-test.hpp"
#include "identity-management-time-fixture.hpp"
#include <boost/filesystem.hpp>

namespace ndn {
namespace pib {
namespace tests {

class PibDbTestFixture : public ndn::security::IdentityManagementTimeFixture
{
public:
  PibDbTestFixture()
    : tmpPath(boost::filesystem::path(TEST_CONFIG_PATH) / "DbTest")
    , db(tmpPath.c_str())
  {
  }

  ~PibDbTestFixture()
  {
    boost::filesystem::remove_all(tmpPath);
  }

  boost::filesystem::path tmpPath;
  PibDb db;
  std::vector<Name> deletedIds;
  std::vector<Name> deletedKeys;
  std::vector<Name> deletedCerts;
  bool isProcessed;
};

BOOST_FIXTURE_TEST_SUITE(TestPibValidator, PibDbTestFixture)

BOOST_AUTO_TEST_CASE(Basic)
{
  PibValidator validator(db);

  Name testUser("/localhost/pib/user/test");
  BOOST_REQUIRE(addIdentity(testUser, RsaKeyParams()));
  Name testUserCertName = m_keyChain.getDefaultCertificateNameForIdentity(testUser);
  shared_ptr<IdentityCertificate> testUserCert = m_keyChain.getCertificate(testUserCertName);

  advanceClocks(time::milliseconds(100));
  Name testUser2("/localhost/pib/user/test2");
  BOOST_REQUIRE(addIdentity(testUser2, RsaKeyParams()));

  db.updateMgmtCertificate(*testUserCert);

  advanceClocks(time::milliseconds(100));
  Name normalId("/normal/id");
  BOOST_REQUIRE(addIdentity(normalId, RsaKeyParams()));
  Name normalIdCertName = m_keyChain.getDefaultCertificateNameForIdentity(normalId);
  shared_ptr<IdentityCertificate> normalIdCert = m_keyChain.getCertificate(normalIdCertName);

  db.addIdentity(normalId);
  db.addKey(normalIdCert->getPublicKeyName(), normalIdCert->getPublicKeyInfo());
  db.addCertificate(*normalIdCert);

  Name command1("/localhost/pib/test/verb/param");
  shared_ptr<Interest> interest1 = make_shared<Interest>(command1);
  m_keyChain.signByIdentity(*interest1, testUser);
  // "test" user is trusted for any command about itself, OK.
  isProcessed = false;
  validator.validate(*interest1,
    [this] (const shared_ptr<const Interest>&) {
      isProcessed = true;
      BOOST_CHECK(true);
    },
    [this] (const shared_ptr<const Interest>&, const std::string&) {
      isProcessed = true;
      BOOST_CHECK(false);
    });
  BOOST_CHECK(isProcessed);

  Name command2("/localhost/pib/test/verb/param");
  shared_ptr<Interest> interest2 = make_shared<Interest>(command2);
  m_keyChain.signByIdentity(*interest2, testUser2);
  // "test2" user is NOT trusted for any command about other user, MUST fail
  isProcessed = false;
  validator.validate(*interest2,
    [this] (const shared_ptr<const Interest>&) {
      isProcessed = true;
      BOOST_CHECK(false);
    },
    [this] (const shared_ptr<const Interest>&, const std::string&) {
      isProcessed = true;
      BOOST_CHECK(true);
    });
  BOOST_CHECK(isProcessed);

  Name command3("/localhost/pib/test/verb/param");
  shared_ptr<Interest> interest3 = make_shared<Interest>(command3);
  m_keyChain.signByIdentity(*interest3, normalId);
  // "normalId" is in "test" pib, can be trusted for some commands about "test".
  // Detail checking is needed, but it is not the job of Validator, OK.
  isProcessed = false;
  validator.validate(*interest3,
    [this] (const shared_ptr<const Interest>&) {
      isProcessed = true;
      BOOST_CHECK(true);
    },
    [this] (const shared_ptr<const Interest>&, const std::string&) {
      isProcessed = true;
      BOOST_CHECK(false);
    });
  BOOST_CHECK(isProcessed);

}

BOOST_AUTO_TEST_SUITE_END()

} // namespace tests
} // namespace pib
} // namespace ndn
