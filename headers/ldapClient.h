#include <string>
#include <ldap.h>

#ifndef LDAPCLIENT_H
#define LDAPCLIENT_H

class LDAPClient
{
    public:
        LDAPClient();
        int authenticateUser(std::string user, const std::string password);

    private:
        const std::string ldapUri = "ldap://ldap.technikum-wien.at:389";
        const int ldapVersion = LDAP_VERSION3;
        LDAP *ldapHandle;
        std::string ldapSearchBaseDomainComponent = "dc=technikum-wien,dc=at";
        std::string ldapBindUser;
        int returnCode = 0;

        bool setUpLDAPConnection();
        bool setVersionOptions();
        bool startTLS();
        bool setUpLDAPClient();

        std::string createBindUser(const std::string username);
        bool bindUser(std::string username, const std::string password);
};

#endif // LDAPCLIENT_H