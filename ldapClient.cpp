#include "ldapClient.h"
#include <iostream>

LDAPClient::LDAPClient()
{
    
}

LDAPClient::~LDAPClient()
{
    ldap_unbind_ext_s(ldapHandle, NULL, NULL);
}
//  0 --> Success
// -1 --> Error
// -2 --> Falied login
int LDAPClient::authenticateUser(const std::string user, const std::string password)
{
    if (setUpLDAPClient() != LDAP_SUCCESS)
    {
        return -1;
    }
    if (bindUser(user, password) != LDAP_SUCCESS)
    {
        // LOGIN_ERROR
        return -2;
    }
    
    return LDAP_SUCCESS;
}
bool LDAPClient::setUpLDAPClient()
{
    if (setUpLDAPConnection() != LDAP_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    if (setVersionOptions() != LDAP_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    if (startTLS() != LDAP_SUCCESS)
    {
        return EXIT_FAILURE;
    }
    return LDAP_SUCCESS;
}
bool LDAPClient::setUpLDAPConnection()
{
    returnCode = ldap_initialize(&ldapHandle, ldapUri.c_str());
    if (returnCode != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_init failed\n");
        return EXIT_FAILURE;
    }
    // TODO: Remove
    printf("connected to LDAP server %s\n", ldapUri.c_str());
    return LDAP_SUCCESS;
}
bool LDAPClient::setVersionOptions()
{
    returnCode = ldap_set_option(
    ldapHandle,
    LDAP_OPT_PROTOCOL_VERSION, // OPTION
    &ldapVersion);             // IN-Value
    if (returnCode != LDAP_OPT_SUCCESS)
    {
        // https://www.openldap.org/software/man.cgi?query=ldap_err2string&sektion=3&apropos=0&manpath=OpenLDAP+2.4-Release
        fprintf(stderr, "ldap_set_option(PROTOCOL_VERSION): %s\n", ldap_err2string(returnCode));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return EXIT_FAILURE;
    }
    return LDAP_SUCCESS;
}
bool LDAPClient::startTLS()
{
    returnCode = ldap_start_tls_s(
    ldapHandle,
    NULL,
    NULL);
    if (returnCode != LDAP_SUCCESS)
    {
        fprintf(stderr, "ldap_start_tls_s(): %s\n", ldap_err2string(returnCode));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return EXIT_FAILURE;
    }
    return LDAP_SUCCESS; 
}
std::string LDAPClient::createBindUser(const std::string user)
{
    return "uid=" + user + ",ou=people,dc=technikum-wien,dc=at";
}
bool LDAPClient::bindUser(std::string user, const std::string password)
{
    BerValue bindCredentials;
    bindCredentials.bv_val = (char *)password.c_str();
    bindCredentials.bv_len = strlen(password.c_str());
    BerValue *servercredp; // server's credentials
    ldapBindUser = createBindUser(user);

    returnCode = ldap_sasl_bind_s(
        ldapHandle,
        ldapBindUser.c_str(),
        LDAP_SASL_SIMPLE,
        &bindCredentials,
        NULL,
        NULL,
        &servercredp);
    if (returnCode != LDAP_SUCCESS)
    {
        fprintf(stderr, "LDAP bind error: %s\n", ldap_err2string(returnCode));
        ldap_unbind_ext_s(ldapHandle, NULL, NULL);
        return EXIT_FAILURE;
    }
    return LDAP_SUCCESS;
}
